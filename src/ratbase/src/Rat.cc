#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
//
#include <G4UImanager.hh>
#include <G4UIterminal.hh>
#include <G4UItcsh.hh>
#include <Randomize.hh>
#include <globals.hh>
#include <G4VisExecutive.hh>
#include <G4UIExecutive.hh>
//
//// time.h must come after Randomize.hh on RedHat 7.3
#include <time.h>
//
#include <RAT/Config.hh>
#include <RAT/Log.hh>
#include <RAT/RunManager.hh>
#include <RAT/InROOTProducer.hh>
#include <RAT/InNetProducer.hh>
#include <RAT/ProcBlock.hh>
#include <RAT/ProcBlockManager.hh>
#include <RAT/PythonProc.hh>
#include <RAT/SignalHandler.hh>
#include <RAT/TrackingMessenger.hh>
#include <RAT/OutROOTProc.hh>
//#include <getopt.h>
#include <TRandom.h>
#include <string>
#include <RAT/Rat.hh>

namespace RAT {

std::string get_short_hostname() {
  char c_hostname[256];
  gethostname(c_hostname, 256);
  std::string hostname(c_hostname);
  std::vector<std::string> parts = split(hostname, ".");
  if( parts.size() > 0 )
    return parts[0];
  else
    return std::string("unknown");
}

std::string get_long_hostname() {
  char c_hostname[256];
  gethostname(c_hostname, 256);
  return std::string(c_hostname);
}

Rat::Rat(AnyParse* parser, int argc, char** argv) : parser(parser), argc(argc), argv(argv) {
  // Setup a base set of arguments
  this->parser->SetHelpLine("[options] macro1.mac macro2.mac ...");
  // Form is AddArgument(name, default, shortname, length, help, type)
  this->parser->AddArgument("run", 0, "r", 1, "Simulated run number", ParseInt);
  this->parser->AddArgument("quiet", false, "q", 0, "Quiet mode, only show warnings", ParseInt);
  this->parser->AddArgument("verbose", false, "v", 0, "Enable verbose printing", ParseInt);
  this->parser->AddArgument("debug", false, "d", 0, "Enable debug printing", ParseInt);
  this->parser->AddArgument("version", false, "V", 0, "Show program version and exit", ParseInt);
  this->parser->AddArgument("database", "", "b", 1, "URL to database", ParseString);
  this->parser->AddArgument("log", "", "l", 1, "Set log filename", ParseString);
  this->parser->AddArgument("seed", -1, "s", 1, "Set random number seed", ParseInt);
  this->parser->AddArgument("input", "", "i", 1, "Set default input filename", ParseString);
  this->parser->AddArgument("output", "", "o", 1, "Set default output filename", ParseString);
  this->parser->AddArgument("vector", "", "x", 1, "Set default vector filename", ParseString);
  this->parser->AddArgument("python", std::vector<std::string>{}, "p", 999, "Set python processors", ParseString);
  this->parser->AddArgument("vis", false, "g", 0, "Load G4UI visualization", ParseInt);
  // Parser setup and ready to read command line arguments
  this->parser->Parse();
  
  if( this->parser->GetValue("database", "") != "" )
    RAT::DB::Get()->SetServer( this->parser->GetValue("database", "") );
  // Database management
  rdb = DB::Get();
  rdb_messenger = new DBMessenger();
  // Local data management
  if( getenv("GLG4DATA") != NULL )
    directories.insert( static_cast<std::string>(getenv("GLG4DATA")) );

}

Rat::~Rat(){
}

void Rat::Begin() {
  this->runTime.Start();
  this->seed = this->parser->GetValue("seed", -1);
  this->input_filename = this->parser->GetValue("input", "");
  this->output_filename = this->parser->GetValue("output", "");
  this->vector_filename = this->parser->GetValue("output", "");
  this->run = this->parser->GetValue("run", 0);
  this->vis = this->parser->GetValue("vis", false);
  this->python_processors = this->parser->GetValue("python", std::vector<std::string>());

  // Logging and logfile
  bool debug_log = this->parser->GetValue("debug", false);
  int display_level = Log::INFO - this->parser->GetValue("quiet", false) + this->parser->GetValue("verbose", false);
  int log_level = debug_log || display_level == Log::DEBUG ? Log::DEBUG : Log::DETAIL;
  std::string logfilename = (std::string("rat.") + get_short_hostname() + "." +
      std::to_string(getpid()) + ".log");
  logfilename = this->parser->GetValue("log", "") != "" ? this->parser->GetValue("log", "") : logfilename;
  Log::Init(logfilename, Log::Level(display_level), Log::Level(log_level));

  // Start by putting all of the basic rat starting functions here, eventually
  // break this apart and fix it up.
  info << "RAT, version x.x.x" << newline;
  info << "Status messages enabled: info ";
  detail << "detail ";
  debug << "debug ";
  info << newline;

  pid_t pid = getpid();
  info << "Hostname: " << get_long_hostname() << " PID: " << pid << newline;

  time_t start_time = time(nullptr);
  if( this->seed == -1 )
    this->seed = start_time ^ (pid << 16);
  detail << "Seeding random number generator: " << this->seed << newline;
  CLHEP::HepRandom::setTheSeed(this->seed);
  // Root ...
  gRandom->SetSeed(this->seed);

  int dbstatus = rdb->LoadDefaults();

  // Run management
  if( this->run > 0 )
  {
    info << "Setting run number: " << this->run << newline;
    rdb->SetDefaultRun(this->run);
  }

  // ... We are really about to put the ENTIRE run process in an exception block ...
  // I'm so so sorry
  try {
    if( this->input_filename != "" )
    {
      rdb->Set("IO", "", "default_input_filename", this->input_filename);
      info << "Setting default input file to " << this->input_filename << newline;
    }
    if( this->output_filename != "" )
    {
      rdb->Set("IO", "", "default_output_filename", this->output_filename);
      info << "Setting default output file to " << this->output_filename << newline;
    }
    if( this->vector_filename != "" )
    {
      rdb->Set("IO", "", "default_vector_filename", this->vector_filename);
      info << "Setting default vector file to " << this->vector_filename << newline;
    }

    // Main analysis block
    ProcBlock* mainBlock = new ProcBlock;
    // Process block manager -- supplies user commands to construct analyis
    // sequence and does the processor creation
    ProcBlockManager* blockManager = new ProcBlockManager(mainBlock);
    TrackingMessenger* trackingMessenger = new TrackingMessenger();

    // Build event producers
    RunManager* runManager = new RunManager(mainBlock);
    InROOTProducer* inroot = new InROOTProducer(mainBlock);
    InNetProducer* innet = new InNetProducer(mainBlock);

    // Setup signal handler to intercept Ctrl-C and quit event loop
    SignalHandler::Init();

    // G4UImanager
    G4UImanager* theUI = G4UImanager::GetUIpointer();

    // Add any python processors specified on the command line ...
    for( auto& v : this->python_processors ){
      Processor* python = new PythonProc;
      python->SetS("class", v);
      mainBlock->DeferAppend(python);
    }

    bool isInteractive = false;
    G4UIExecutive* theSession = nullptr;
    if( this->vis )
      theSession = new G4UIExecutive(this->argc, this->argv);

    std::string command = "/control/execute ";
    for(auto& mac : this->parser->Positionals){
      std::ifstream macro(mac);
      std::string macroline;
      while( macro.good() )
      {
        getline(macro, macroline);
        Log::AddMacro(macroline + "\n");
      }
      theUI->ApplyCommand(command + mac);
    }
    if( this->vis )
      theSession->SessionStart();
    delete theSession;

    // Hack to close GLG4sim output file if used
    theUI->ApplyCommand("/event/output_file");

    delete blockManager;
    delete mainBlock;
    delete runManager;
    delete inroot;
    delete innet;
    delete rdb_messenger;
    delete trackingMessenger;
  }
  catch ( DBNotFoundError &e )
  {
    Log::Die("DB: Field " + e.table + "[" + e.index + "]." + e.field
        + " lookup failure. Does not exist or has wrong type.");
  }
}

void Rat::Report(){
  struct rusage usage;
  getrusage(RUSAGE_SELF, &usage);
  double usertime = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1e6;
  double systime = usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1e6;

  printf("Run time: %8.2f sec\n", runTime.RealTime());
  printf("CPU usage: user %8.2f sec\tsys %6.2f sec\n", usertime, systime);
}

} // namespace RAT
