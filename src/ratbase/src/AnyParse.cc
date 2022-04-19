#include <RAT/AnyParse.hh>
#include <algorithm>

namespace RAT {

AnyParse::AnyParse(int argc, char** argv)
{
  this->CommandLine = std::vector<std::string>(argv+1,argv+argc);
}

void AnyParse::Parse(){
  std::string key = "NULL";
  int remainingArgs = 0;
  std::any selection;
  std::vector<std::string> subArguments;
  for( auto& arg : this->CommandLine )
  {
    if( arg.rfind("--", 0) == 0 )
    {
      arg.erase(0, 2);
      key = arg;
      //selection = Arguments[key];
      remainingArgs = nargs[key];
      subArguments.clear();
      // If something takes no additional arguments then it is a switch
      if( remainingArgs == 0 )
        this->SetValue(key, true);
    }
    else if ( arg.rfind("-", 0) == 0 )
    {
      arg.erase(0, 1);
      key = this->shortName[arg];
      //selection = Arguments[key];
      remainingArgs = nargs[key];
      subArguments.clear();
      if( remainingArgs == 0 )
        this->SetValue(key, true);
    }
    else
    {
      // Collect the arguments for the selected key up to nargs
      if( remainingArgs > 1 )
      {
        subArguments.push_back( arg );
        remainingArgs--;
      }
      else if ( remainingArgs == 1 )
      {
        // Process the list of arguments
        subArguments.push_back( arg );
        remainingArgs--;
        // Now can I convert?
        if( subArguments.size() == 1 )
        {
          ConvertType cv = this->Conversion[key];
          if( cv == ParseInt )
          {
            this->SetValue(key, stoi(arg));
          }
          else if( cv == ParseDouble )
          {
            this->SetValue(key, stod(arg));
          }
          else
            this->SetValue(key, arg);
        }
        else
        {
          ConvertType cv = this->Conversion[key];
          if( cv == ParseInt )
          {
            std::vector<int> vint;
            std::transform( subArguments.begin(), subArguments.end(), std::back_inserter(vint),
                [&](std::string s) { return stoi(s); } );
            this->SetValue(key, vint);
          }
          else if( cv == ParseDouble )
          {
            std::vector<double> vdouble;
            std::transform( subArguments.begin(), subArguments.end(), std::back_inserter(vdouble),
                [&](std::string s) { return stod(s); } );
            this->SetValue(key, vdouble);
          }
          else
            this->SetValue(key, subArguments);
        }
      }
      else
      {
        this->Positionals.push_back(arg);
      }
    }
  }
}

} // namespace RAT
