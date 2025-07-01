// Run the macro using:
// root example_root_macro.C

// Root macro to open a root file, 
// extract nhits for each event, and plot the NHits distribution

void example_root_macro() {

    const char* root_file_name = "example.ntuple.root"; // Change the path to the file as needed

    TFile *file = TFile::Open(root_file_name);  // Open the ROOT file
    TTree *output_tree = (TTree*)file->Get("output"); // Get the TTree from the file

    // Extracting the desired quantity from the root tree
    int nhits;
    output_tree->SetBranchAddress("nhits", &nhits);

    std::vector<int> *mcPMTID = nullptr;
    output_tree->SetBranchAddress("mcPMTID", &mcPMTID); 

    Long64_t Events = output_tree->GetEntriesFast(); // Total number of entries in the tree

    TH1D* nhits_hist = new TH1D("nhits_hist","NHits Distribution", 30, 0.0, 30.0); // Initializing the histogram

    // Loop over events in the tree
    for (int i = 0; i < Events; i++) { 
        output_tree->GetEntry(i); // Retrieve the i-th entry
        nhits_hist->Fill(nhits); // Fill the histogram

        // Print MC PMT IDs for the first 10 events
        if (i < 10) {
            std::cout << "Entry " << i << ": ";
            for (int j = 0; j < mcPMTID->size(); j++) {
                std::cout << (*mcPMTID)[j] << ", ";
            }
            std::cout << std::endl;
        }
    }

    // Plotting
    TCanvas *c1 = new TCanvas("c1", "", 800, 600);
    c1->cd();
    nhits_hist->GetXaxis()->SetTitle("Number of PMT hits per event");
    nhits_hist->GetYaxis()->SetTitle("Events");
    nhits_hist->SetTitle("NHits distribution");
    nhits_hist->Draw();
}