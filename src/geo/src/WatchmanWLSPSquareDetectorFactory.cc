#include <RAT/WatchmanWLSPSquareDetectorFactory.hh>
#include <RAT/Log.hh>
#include <RAT/DB.hh>

#include <math.h>
#include <vector>

using namespace std;

namespace RAT {

    void WatchmanWLSPSquareDetectorFactory::DefineDetector(DBLinkPtr /*detector*/) {
        DB *db = DB::Get();
        DBLinkPtr params = db->GetLink("WATCHMAN_PARAMS");
        const double photocathode_coverage = params->GetD("photocathode_coverage");
        const double veto_coverage = params->GetD("veto_coverage");
        const double veto_offset = 700;
        const std::string geo_template = "Watchman_WLSP/Watchman_WLSP.geo";
        if (db->Load(geo_template) == 0) {
            Log::Die("WatchmanDetectorFactory: could not load template Watchman_WLSP/Watchman_WLSP.geo");
        }

        //calculate the area of the defined inner_pmts
        DBLinkPtr inner_pmts = db->GetLink("GEO","inner_pmts");
        string pmt_model = inner_pmts->GetS("pmt_model");
        DBLinkPtr pmt = db->GetLink("PMT", pmt_model);
        vector<double> rho_edge = pmt->GetDArray("rho_edge");
        vector<double> z_edge = pmt->GetDArray("z_edge");
        double photocathode_radius = rho_edge[0];
        for (size_t i = 1; i < rho_edge.size(); i++) {
            if (photocathode_radius < rho_edge[i]) photocathode_radius = rho_edge[i];
        }
        const double photocathode_area = M_PI*photocathode_radius*photocathode_radius;

        DBLinkPtr shield              = db->GetLink("GEO","shield");
        const double steel_thickness  = shield->GetD("steel_thickness");
        const double veto_thickness_r = shield->GetD("veto_thickness_r");//Distance between TANK and Inner PMT
        const double detector_size_d  = shield->GetD("detector_size_d");
	      const double veto_thickness_z = shield->GetD("veto_thickness_z");//Distance between TANK and Inner PMT
        const double detector_size_z  = shield->GetD("detector_size_z");

        const double cable_radius = detector_size_d/2.0 - veto_thickness_r + 4.0*steel_thickness;
        const double pmt_radius = detector_size_d/2.0 - veto_thickness_r - 4.0*steel_thickness;
        const double veto_radius = pmt_radius + veto_offset;

        const double topbot_offset = detector_size_z/2.0 - veto_thickness_z;
        const double topbot_veto_offset = topbot_offset + veto_offset;

        const double surface_area = 2.0*M_PI*pmt_radius*pmt_radius + 2.0*topbot_offset*2.0*M_PI*pmt_radius;
        const double required_pmts = ceil(photocathode_coverage * surface_area / photocathode_area);
        const double veto_surface_area = 2.0*M_PI*veto_radius*veto_radius + 2.0*topbot_veto_offset*2.0*M_PI*veto_radius;
        const double required_vetos = ceil(veto_coverage * veto_surface_area / photocathode_area);

        const double pmt_space = sqrt(surface_area/required_pmts);
        const double veto_space = sqrt(veto_surface_area/required_vetos);

        const size_t cols = round(2.0*M_PI*pmt_radius/pmt_space);
        const size_t rows = round(2.0*topbot_offset/pmt_space);
        const size_t veto_cols = round(2.0*M_PI*veto_radius/veto_space);
        const size_t veto_rows = round(2.0*topbot_veto_offset/veto_space);
        
        size_t num_pmts;
        size_t num_vetos;
        size_t total_pmts;
        vector<double> cable_x(cols), cable_y(cols);
        vector<pair<int,int> > topbot;
        vector<pair<int,int> > topbot_veto;
        if (photocathode_coverage != 0.00) { //Setting photocathode_coverage to 0 will use the baseline design -- YOU SHOULD STILL USE THIS FACTORY TO PROPERLY GENERATE WLS PLATES AND THEIR REFLECTORS
          info << "Generating new PMT positions for:\n";
          info << "\tdesired photocathode coverage " << photocathode_coverage << '\n';
          info << "\ttotal area " << surface_area << '\n';
          info << "\tphotocathode radius " << photocathode_radius << '\n';
          info << "\tphotocathode area " << photocathode_area << '\n';
          info << "\tdesired PMTs " << required_pmts << '\n';
          info << "\tPMT spacing " << pmt_space << '\n';
        

          //make the grid for top and bottom PMTs

          const int rdim = round(pmt_radius/pmt_space);
          for (int i = -rdim; i <= rdim; i++) {
              for (int j = -rdim; j <= rdim; j++) {
                  if (pmt_space*sqrt(i*i+j*j) <= pmt_radius-pmt_space/2.0) {
                      topbot.push_back(make_pair(i,j));
                  }
                  if (veto_space*sqrt(i*i+j*j) <= pmt_radius-pmt_space/2.0) { // pmt_* is not a mistake
                      topbot_veto.push_back(make_pair(i,j));
                  }
              }
          }

          num_pmts = cols*rows + 2*topbot.size();
          num_vetos = veto_cols*veto_rows + 2*topbot_veto.size();
          total_pmts = num_pmts + num_vetos;

          info << "Actual calculated values:\n";
          info << "\tactual photocathode coverage " << photocathode_area*num_pmts/surface_area << '\n';
          info << "\tgenerated PMTs " << num_pmts << '\n';
          info << "\tcols " << cols << '\n';
          info << "\trows " << rows << '\n';
          info << "\tgenerated Vetos " << num_vetos << '\n';
          info << "\tcols " << veto_cols << '\n';
          info << "\trows " << veto_rows << '\n';
          
        }
        else info<<"Using blessed baseline design geo file.\n";
        

        
        DBLinkPtr wlsp                = db->GetLink("GEO","WLS_Plates");
        vector <double> pmtinfox;
        vector <double> pmtinfoy;
        vector <double> pmtinfoz;
        vector <double> pmtinfodirx;
        vector <double> pmtinfodiry;
        vector <double> pmtinfodirz;
        DBLinkPtr pmtinfo = db->GetLink("PMTINFO");
        pmtinfox = pmtinfo->GetDArray("x");
        pmtinfoy = pmtinfo->GetDArray("y");
        pmtinfoz = pmtinfo->GetDArray("z");
        pmtinfodirx = pmtinfo->GetDArray("dir_x");
        pmtinfodiry = pmtinfo->GetDArray("dir_y");
        pmtinfodirz = pmtinfo->GetDArray("dir_z");
        if (photocathode_coverage == 0.00) total_pmts = pmtinfox.size();
        vector<double> x(total_pmts), y(total_pmts), z(total_pmts), dir_x(total_pmts), dir_y(total_pmts), dir_z(total_pmts);
        vector<double> xp, yp, zp;
        vector<double> dir_xp, dir_yp, dir_zp;
        vector<int> type(total_pmts);
        
        //WLS Plate positions are generated alongside the PMT positions here
        if (photocathode_coverage == 0.00) {
          
          for (size_t i = 0; i < total_pmts; i++) {
            x[i] = pmtinfox[i];
            y[i] = pmtinfoy[i];
            z[i] = pmtinfoz[i];
             
            dir_x[i] = pmtinfodirx[i];
            dir_y[i] = pmtinfodiry[i];
            dir_z[i] = pmtinfodirz[i];
                
            xp.push_back(x[i]+(dir_x[i]*z_edge[1]));
            yp.push_back(y[i]+(dir_y[i]*z_edge[1]));
            zp.push_back(z[i]+(dir_z[i]*z_edge[1]));
            dir_xp.push_back(pmtinfodirx[i]);
            dir_yp.push_back(pmtinfodiry[i]);
            dir_zp.push_back(pmtinfodirz[i]);
                
            type[i] = 1;
          }
        }
        else {
          
          //generate cylinder PMT positions
          for (size_t col = 0; col < cols; col++) {
              for (size_t row = 0; row < rows; row++) {
                  const size_t idx = row + col*rows;
                  const double phi = 2.0*M_PI*(col+0.5)/cols;

                  x[idx] = pmt_radius*cos(phi);
                  y[idx] = pmt_radius*sin(phi);
                  z[idx] = row*2.0*topbot_offset/rows + pmt_space/2.0 - topbot_offset;
                 
                  dir_x[idx] = -cos(phi);
                  dir_y[idx] = -sin(phi);
                  dir_z[idx] = 0.0;
                
                  xp.push_back(x[idx]+(dir_x[idx]*z_edge[1]));
                  yp.push_back(y[idx]+(dir_y[idx]*z_edge[1]));
                  zp.push_back(z[idx]+(dir_z[idx]*z_edge[1]));
                  dir_xp.push_back(dir_x[idx]);
                  dir_yp.push_back(dir_y[idx]);
                  dir_zp.push_back(dir_z[idx]);
                
                  type[idx] = 1;
              }
          }

          //generate topbot PMT positions
          for (size_t i = 0; i < topbot.size(); i++) {
              const size_t idx = rows*cols+i*2;

              //top = idx
              x[idx] = pmt_space*topbot[i].first;
              y[idx] = pmt_space*topbot[i].second;
              z[idx] = topbot_offset;

              dir_x[idx] = dir_y[idx] = 0.0;
              dir_z[idx] = -1.0;

              type[idx] = 1;
  
              xp.push_back(x[idx]+(dir_x[idx]*z_edge[1]));
              yp.push_back(y[idx]+(dir_y[idx]*z_edge[1]));
              zp.push_back(z[idx]+(dir_z[idx]*z_edge[1]));
              dir_xp.push_back(dir_x[idx]);
              dir_yp.push_back(dir_y[idx]);
              dir_zp.push_back(dir_z[idx]);
            
              //bot = idx+1
              x[idx+1] = pmt_space*topbot[i].first;
              y[idx+1] = pmt_space*topbot[i].second;
              z[idx+1] = -topbot_offset;

              dir_x[idx+1] = dir_y[idx] = 0.0;
              dir_z[idx+1] = 1.0;
            
              xp.push_back(x[idx+1]+(dir_x[idx+1]*z_edge[1]));
              yp.push_back(y[idx+1]+(dir_y[idx+1]*z_edge[1]));
              zp.push_back(z[idx+1]+(dir_z[idx+1]*z_edge[1]));
              dir_xp.push_back(dir_x[idx+1]);
              dir_yp.push_back(dir_y[idx+1]);
              dir_zp.push_back(dir_z[idx+1]);
            
              type[idx+1] = 1;
          }

          //generate cylinder Veto positions
          for (size_t col = 0; col < veto_cols; col++) {
            for (size_t row = 0; row < veto_rows; row++) {
                const size_t idx = num_pmts + row + col*veto_rows;
                const double phi = 2.0*M_PI*col/veto_cols;

                x[idx] = veto_radius*cos(phi);
                y[idx] = veto_radius*sin(phi);
                z[idx] = row*2.0*topbot_offset/veto_rows + veto_space/2 - topbot_offset;

                dir_x[idx] = cos(phi);
                dir_y[idx] = sin(phi);
                dir_z[idx] = 0.0;

                type[idx] = 2;
            }
          }

          //generate topbot Veto positions
          for (size_t i = 0; i < topbot_veto.size(); i++) {
              const size_t idx = num_pmts + veto_rows*veto_cols+i*2;
  
              //top = idx
              x[idx] = veto_space*topbot_veto[i].first;
              y[idx] = veto_space*topbot_veto[i].second;
              z[idx] = topbot_veto_offset;

              dir_x[idx] = dir_y[idx] = 0.0;
              dir_z[idx] = 1.0;

              type[idx] = 2;

              //bot = idx+1
              x[idx+1] = veto_space*topbot_veto[i].first;
              y[idx+1] = veto_space*topbot_veto[i].second;
              z[idx+1] = -topbot_veto_offset;

              dir_x[idx+1] = dir_y[idx] = 0.0;
              dir_z[idx+1] = -1.0;

              type[idx+1] = 2;
          }

          //generate cable positions
          
          for (size_t col = 0; col < cols; col++) {
              cable_x[col] = cable_radius*cos(col*2.0*M_PI/cols);
              cable_y[col] = cable_radius*sin(col*2.0*M_PI/cols);
          }


        
          info << "Update geometry fields related to the reflective and absorptive tarps...\n";
          // Side tarps
          db->SetD("GEO","white_sheet_side","r_max",veto_radius+60.0);
          db->SetD("GEO","white_sheet_side","r_min",veto_radius-10.0+60.0); // Marc Bergevin: Hardcoding in a 1 cm value for tickness
          db->SetD("GEO","white_sheet_side","size_z",topbot_veto_offset);
          db->SetD("GEO","black_sheet_side","r_max",pmt_radius+10.0+60.0);
          db->SetD("GEO","black_sheet_side","r_min",pmt_radius+60.0); // Marc Bergevin: Hardcoding in a 1 cm value for tickness
          db->SetD("GEO","black_sheet_side","size_z",topbot_offset);
          
          db->SetD("GEO","Rod_assemblies","r_max",(pmt_radius+300.)); // Based on Geofile thickness values of 10 cm
          db->SetD("GEO","Rod_assemblies","r_min",(pmt_radius+200.));
          db->SetD("GEO","Rod_assemblies","size_z",topbot_offset);

          db->SetD("GEO","white_sheet_tank_side","r_max",detector_size_d/2.0 -10.0+60.0);
          db->SetD("GEO","white_sheet_tank_side","r_min",detector_size_d/2.0 -35.0+60.0);
          db->SetD("GEO","white_sheet_tank_side","size_z",detector_size_z/2.0-35.0+60.0);
        
        
          //Top tarps
          vector<double> move_white_top;
          move_white_top.push_back(0.0);
          move_white_top.push_back(0.0);
          move_white_top.push_back(topbot_veto_offset);
          vector<double> move_black_top;
          move_black_top.push_back(0.0);
          move_black_top.push_back(0.0);
          move_black_top.push_back(topbot_offset);
          vector<double> move_topcap;
          move_topcap.push_back(0.0);
          move_topcap.push_back(0.0);
          move_topcap.push_back(topbot_offset+200.);
          vector<double> move_toptruss;
          move_toptruss.push_back(0.0);
          move_toptruss.push_back(0.0);
          move_toptruss.push_back(topbot_offset+200.+2.5);// Bergevin: Values based on geofile
        
          vector<double> move_toptanktarp;
          move_toptanktarp.push_back(0.0);
          move_toptanktarp.push_back(0.0);
          move_toptanktarp.push_back(detector_size_z/2.0-30.0);// Bergevin: Values based on geofile
        
        
        
        
        
        
          db->SetD("GEO","white_sheet_top","r_max",veto_radius);
          db->SetDArray("GEO","white_sheet_top","position",move_white_top);
          db->SetD("GEO","black_sheet_top","r_max",pmt_radius);
          db->SetDArray("GEO","black_sheet_top","position",move_black_top);
          db->SetD("GEO","Top_cap_framework","r_max",pmt_radius);
          db->SetDArray("GEO","Top_cap_framework","position",move_topcap);
          db->SetD("GEO","Wall_support_truss_top","r_min",pmt_radius+5.0);  // Bergevin: Values based
          db->SetD("GEO","Wall_support_truss_top","r_max",pmt_radius+200.0);// on geofile
          db->SetDArray("GEO","Wall_support_truss_top","position",move_toptruss);
    
          db->SetD("GEO","white_sheet_tank_top","r_max",detector_size_d/2.0 -35.0);
          db->SetDArray("GEO","white_sheet_tank_top","position",move_toptanktarp);
        
        
        
        
          //Bottom tarps
          vector<double> move_white_bottom;
          move_white_bottom.push_back(0.0);
          move_white_bottom.push_back(0.0);
          move_white_bottom.push_back(-topbot_veto_offset);
          vector<double> move_black_bottom;
          move_black_bottom.push_back(0.0);
          move_black_bottom.push_back(0.0);
          move_black_bottom.push_back(-topbot_offset);
          vector<double> move_bottomcap;
          move_bottomcap.push_back(0.0);
          move_bottomcap.push_back(0.0);
          move_bottomcap.push_back(-topbot_offset-200.);
          vector<double> move_bottomtruss;
          move_bottomtruss.push_back(0.0);
          move_bottomtruss.push_back(0.0);
          move_bottomtruss.push_back(-topbot_offset-200.-2.5);// Bergevin: Values based on geofile
        
          vector<double> move_bottomtanktarp;
          move_bottomtanktarp.push_back(0.0);
          move_bottomtanktarp.push_back(0.0);
          move_bottomtanktarp.push_back(-detector_size_z/2.0+30.0);// Bergevin: Values based on geofile
        
        
        
          db->SetD("GEO","white_sheet_bottom","r_max",veto_radius);
          db->SetDArray("GEO","white_sheet_bottom","position",move_white_bottom);
          db->SetD("GEO","black_sheet_bottom","r_max",pmt_radius);
          db->SetDArray("GEO","black_sheet_bottom","position",move_black_bottom);
          db->SetD("GEO","Bottom_cap_framework","r_max",pmt_radius);
          db->SetDArray("GEO","Bottom_cap_framework","position",move_bottomcap);
          db->SetD("GEO","Wall_support_truss_bottom","r_min",pmt_radius+5.0);  // Bergevin: Values based
          db->SetD("GEO","Wall_support_truss_bottom","r_max",pmt_radius+200.0);// on geofile
          db->SetDArray("GEO","Wall_support_truss_bottom","position",move_bottomtruss);
    
          db->SetD("GEO","white_sheet_tank_bottom","r_max",detector_size_d/2.0 -35.0);
          db->SetDArray("GEO","white_sheet_tank_bottom","position",move_bottomtanktarp);
        
        
          info << "Adjusting the Bottom cap standoff frames ...\n";

        
          DBLinkPtr frame_0              = db->GetLink("GEO","Bottom_cap_standoff_frame_0");
          vector<double> standoff_frame_0_size = frame_0->GetDArray("size");
          vector<double> standoff_frame_0_pos = frame_0->GetDArray("position");
          DBLinkPtr frame_1              = db->GetLink("GEO","Bottom_cap_standoff_frame_1");
          vector<double> standoff_frame_1_size = frame_1->GetDArray("size");
          vector<double> standoff_frame_1_pos = frame_1->GetDArray("position");
          DBLinkPtr frame_2              = db->GetLink("GEO","Bottom_cap_standoff_frame_2");
          vector<double> standoff_frame_2_size = frame_2->GetDArray("size");
          vector<double> standoff_frame_2_pos = frame_2->GetDArray("position");
          DBLinkPtr frame_3              = db->GetLink("GEO","Bottom_cap_standoff_frame_3");
          vector<double> standoff_frame_3_size = frame_3->GetDArray("size");
          vector<double> standoff_frame_3_pos = frame_3->GetDArray("position");
          DBLinkPtr frame_4              = db->GetLink("GEO","Bottom_cap_standoff_frame_4");
          vector<double> standoff_frame_4_size = frame_4->GetDArray("size");
          vector<double> standoff_frame_4_pos = frame_4->GetDArray("position");
        
          info << "Size loaded in frame 0" << standoff_frame_0_size[0] << " " << standoff_frame_0_size[1] << " " << standoff_frame_0_size[2] << "...\n";
          if(standoff_frame_0_size[2] != (detector_size_z/2.0 - (topbot_offset+200.+2.5))){
              standoff_frame_0_size[2] = (detector_size_z/2.0 - (topbot_offset+200.+2.5))/2.0;
              standoff_frame_0_pos[2]  = -(detector_size_z/2.0 + (topbot_offset+200.+2.5))/2.0;
              info << "New size " << standoff_frame_0_size[0] << " " << standoff_frame_0_size[1] << " " << standoff_frame_0_size[2] << "...\n";
          }
          info << "Size loaded in frame 1" << standoff_frame_1_size[0] << " " << standoff_frame_1_size[1] << " " << standoff_frame_1_size[2] << "...\n";
          if(standoff_frame_1_size[2] != (detector_size_z/2.0 - (topbot_offset+200.+2.5))){
              standoff_frame_1_size[2] = (detector_size_z/2.0 - (topbot_offset+200.+2.5))/2.0;
              standoff_frame_1_pos[2]  = -(detector_size_z/2.0 + (topbot_offset+200.+2.5))/2.0;
              info << "New size " << standoff_frame_1_size[0] << " " << standoff_frame_1_size[1] << " " << standoff_frame_1_size[2] << "...\n";
          }
          info << "Size loaded in frame 2" << standoff_frame_2_size[0] << " " << standoff_frame_2_size[1] << " " << standoff_frame_2_size[2] << "...\n";
          if(standoff_frame_2_size[2] != (detector_size_z/2.0 - (topbot_offset+200.+2.5))){
              standoff_frame_2_size[2] = (detector_size_z/2.0 - (topbot_offset+200.+2.5))/2.0;
              standoff_frame_2_pos[2]  = -(detector_size_z/2.0 + (topbot_offset+200.+2.5))/2.0;
              info << "New size " << standoff_frame_2_size[0] << " " << standoff_frame_2_size[1] << " " << standoff_frame_2_size[2] << "...\n";
          }
          info << "Size loaded in frame 3" << standoff_frame_3_size[0] << " " << standoff_frame_3_size[1] << " " << standoff_frame_3_size[2] << "...\n";
          if(standoff_frame_3_size[2] != (detector_size_z/2.0 - (topbot_offset+200.+2.5))){
              standoff_frame_3_size[2] = (detector_size_z/2.0 - (topbot_offset+200.+2.5))/2.0;
              standoff_frame_3_pos[2]  = -(detector_size_z/2.0 + (topbot_offset+200.+2.5))/2.0;
              info << "New size " << standoff_frame_3_size[0] << " " << standoff_frame_3_size[1] << " " << standoff_frame_3_size[2] << "...\n";
          }
          info << "Size loaded in frame 4" << standoff_frame_4_size[0] << " " << standoff_frame_4_size[1] << " " << standoff_frame_4_size[2] << "...\n";
          if(standoff_frame_4_size[2] != (detector_size_z/2.0 - (topbot_offset+200.+2.5))){
              standoff_frame_4_size[2] = (detector_size_z/2.0 - (topbot_offset+200.+2.5))/2.0;
              standoff_frame_4_pos[2]  = -(detector_size_z/2.0 + (topbot_offset+200.+2.5))/2.0;
              info << "New size " << standoff_frame_4_size[0] << " " << standoff_frame_4_size[1] << " " << standoff_frame_4_size[2] << "...\n";
          }
 
          db->SetDArray("GEO","Bottom_cap_standoff_frame_0","size",standoff_frame_0_size);
          db->SetDArray("GEO","Bottom_cap_standoff_frame_0","position",standoff_frame_0_pos);
          db->SetDArray("GEO","Bottom_cap_standoff_frame_1","size",standoff_frame_1_size);
          db->SetDArray("GEO","Bottom_cap_standoff_frame_1","position",standoff_frame_1_pos);
          db->SetDArray("GEO","Bottom_cap_standoff_frame_2","size",standoff_frame_2_size);
          db->SetDArray("GEO","Bottom_cap_standoff_frame_2","position",standoff_frame_2_pos);
          db->SetDArray("GEO","Bottom_cap_standoff_frame_3","size",standoff_frame_3_size);
          db->SetDArray("GEO","Bottom_cap_standoff_frame_3","position",standoff_frame_3_pos);
          db->SetDArray("GEO","Bottom_cap_standoff_frame_4","size",standoff_frame_4_size);
          db->SetDArray("GEO","Bottom_cap_standoff_frame_4","position",standoff_frame_4_pos);
          info << "Override default PMTINFO information...\n";
          db->SetDArray("PMTINFO","x",x);
          db->SetDArray("PMTINFO","y",y);
          db->SetDArray("PMTINFO","z",z);
          db->SetDArray("PMTINFO","dir_x",dir_x);
          db->SetDArray("PMTINFO","dir_y",dir_y);
          db->SetDArray("PMTINFO","dir_z",dir_z);
          db->SetIArray("PMTINFO","type",type);
        }
        
        info << "Override default WLSPINFO information...\n";

        db->SetDArray("WLSPINFO","x",xp);
        db->SetDArray("WLSPINFO","y",yp);
        db->SetDArray("WLSPINFO","z",zp);
        db->SetDArray("WLSPINFO","dir_x",dir_xp);
        db->SetDArray("WLSPINFO","dir_y",dir_yp);
        db->SetDArray("WLSPINFO","dir_z",dir_zp);
        db->SetIArray("WLSPINFO","type",type);
        
        if (photocathode_coverage != 0.00) {
          info << "Update geometry fields related to the reflective and absorptive tarps...\n";
        
        
          info << "Update geometry fields related to veto PMTs...\n";
          db->SetI("GEO","shield","veto_start",num_pmts);
          db->SetI("GEO","shield","veto_len",num_vetos);
          db->SetI("GEO","veto_pmts","start_idx",num_pmts);
          db->SetI("GEO","veto_pmts","end_idx",total_pmts-1);

          info << "Update geometry fields related to normal PMTs...\n";
          db->SetI("GEO","shield","cols",cols);
          db->SetI("GEO","shield","rows",rows);
          db->SetI("GEO","shield","inner_start",0);
          db->SetI("GEO","shield","inner_len",num_pmts);
          db->SetI("GEO","inner_pmts","start_idx",0);
          db->SetI("GEO","inner_pmts","end_idx",num_pmts-1);

          info << "Update cable positions to match shield...\n";
          db->SetDArray("cable_pos","x",cable_x);
          db->SetDArray("cable_pos","y",cable_y);
          db->SetDArray("cable_pos","z",vector<double>(cols,0.0));
          db->SetDArray("cable_pos","dir_x",vector<double>(cols,0.0));
          db->SetDArray("cable_pos","dir_y",vector<double>(cols,0.0));
          db->SetDArray("cable_pos","dir_z",vector<double>(cols,1.0));

          //This generates the geometry values for the WLS plates and their reflectors
          db->SetDArray("GEO","WLS_Plates","size",{(pmt_space/2.0)-10.0,(pmt_space/2.0)-10.0,wlsp->GetDArray("z")[1]});
          db->SetDArray("GEO","WLS_Plates","r_max",{rho_edge[1]+5.0,rho_edge[1]-15.0});
          db->SetDArray("GEO","WLS_Plates","r_min",{0.0,0.0});
          db->SetDArray("GEO","WLS_Plates","z",{-wlsp->GetDArray("z")[1]-1.0,wlsp->GetDArray("z")[1]+1.0});
          db->SetDArray("GEO","WLSP_reflector","inner_size",{(pmt_space/2.0)-10.0,(pmt_space/2.0)-10.0,wlsp->GetDArray("z")[1]+2.0});
          db->SetDArray("GEO","WLSP_reflector","outer_size",{(pmt_space/2.0)-10.0+2.0,(pmt_space/2.0)-10.0+2.0,wlsp->GetDArray("z")[1]});
        }
        else {
          //If the baseline detector configuration is used, more constant values are used.  I still do this here rather than in the geo file because it reads in the PMT dimensions
          db->SetDArray("GEO","WLS_Plates","size",{240.0,240.0,wlsp->GetDArray("z")[1]});
          db->SetDArray("GEO","WLS_Plates","r_max",{rho_edge[1]+5.0,rho_edge[1]-15.0});
          db->SetDArray("GEO","WLS_Plates","r_min",{0.0,0.0});
          db->SetDArray("GEO","WLS_Plates","z",{-wlsp->GetDArray("z")[1]-1.0,wlsp->GetDArray("z")[1]+1.0});
          db->SetDArray("GEO","WLSP_reflector","inner_size",{240.0,240.0,wlsp->GetDArray("z")[1]+2.0});
          db->SetDArray("GEO","WLSP_reflector","outer_size",{242.0,242.0,wlsp->GetDArray("z")[1]});
        }
        
        //db->SetDArray("GEO","WLSP_reflector","inner_size",{(pmt_space/2.0)-8.0,(pmt_space/2.0)-8.0,wlspcover->GetDArray("z")[1]});
        //db->SetDArray("GEO","WLSP_reflector","outer_size",{(pmt_space/2.0)-9.0,(pmt_space/2.0)-9.0,wlspcover->GetDArray("z")[1]+1});
        
        

        if (photocathode_coverage != 0.00) {
          //DBLinkPtr inner_pmts = db->GetLink("GEO","inner_pmts");
          //  const vector<double> &size = table->GetDArray("boxsize");
          DBLinkPtr cavern = db->GetLink("GEO","cavern");
          // const vector<double>  &cavSize = cavern->GetDArray("size_z"); //Should be a cube
          // float _shift = cavSize[0]-detector_size_z/2.0;
          const double  cavSize = cavern->GetD("size_z"); //Should be a cube
          float _shift = cavSize-detector_size_z/2.0;

          if(_shift<0.0){
            info << "size of detector greater than cavern. (" << detector_size_z << " mm," << cavSize*2 <<"\n";
          }
          vector<double> shift,minshift;
          shift.push_back(0.0);
          shift.push_back(0.0);
          shift.push_back(_shift);
          minshift.push_back(0.0);
          minshift.push_back(0.0);
          minshift.push_back(-_shift);
          info << "Update height of rock and cavern air... (" << _shift << " mm shift)\n";
  
          db->SetDArray("GEO","rock_1",  "position",shift);
          // db->SetDArray("GEO","cavern",  "position",noshift);
        
          info << "Adjust size and position of tank...\n";
          db->SetD("GEO","tank","r_max",detector_size_d/2.0);
          db->SetD("GEO","tank","size_z",detector_size_z/2.0);
          db->SetDArray("GEO","tank","position",minshift);
          // db->SetDArray("GEO","detector","position",minshift);
        }
        
    }

}
