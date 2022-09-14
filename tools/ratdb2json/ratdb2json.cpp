#include <RAT/AnyParse.hh>
#include <RAT/DB.hh>
#include <RAT/DBTable.hh>
#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char **argv) {
  auto parser = new RAT::AnyParse(argc, argv);
  parser->SetHelpLine("[options] inputfile.ratdb");
  parser->AddArgument("output", "", "o", 1, "Output json file", RAT::ParseString);
  parser->AddArgument("verbose", false, "v", 0, "Verbosity", RAT::ParseInt);
  parser->Parse();

  json::Value jsontable(json::TOBJECT);
  for (auto &filename : parser->Positionals) {
    if (parser->GetValue("verbose", false)) std::cout << "Reading " << filename << std::endl;
    std::vector<RAT::DBTable *> tables = RAT::DB::ReadRATDBFile(filename);
    for (auto &t : tables) {
      auto tjson = t->GetCompleteJSON();
      std::string name = tjson.getMember("name").getString();
      std::string index = tjson.isMember("index") ? tjson.getMember("index").getString() : "default";
      if (parser->GetValue("verbose", false)) std::cout << " > [" << name << "] (" << index << ")\n";
      if (jsontable.isMember(name)) {
        jsontable.getMember(name).setMember(index, t->GetCompleteJSON());
      } else {
        json::Value subtable;
        subtable.setMember(index, t->GetCompleteJSON());
        jsontable.setMember(name, subtable);
      }
      // jsontable.setMember(t->GetName(), t->GetCompleteJSON());
    }
  }

  // Write to stream
  if (parser->GetValue("output", "") != "") {
    std::ofstream outputfile(parser->GetValue("output", ""));
    json::Writer writer(outputfile);
    writer.putValue(jsontable);
  } else {
    json::Writer writer(std::cout);
    writer.putValue(jsontable);
  }
}
