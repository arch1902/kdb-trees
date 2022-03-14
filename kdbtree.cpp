#include<iostream>
#include "file_manager.h"
#include "errors.h"
#include<cstring>
#include<bits/stdc++.h>

using namespace std;

struct Node{
	int node_type; // 0 for point page, 1 for region page
	int split_dim; 
	int page_identifier;
    vector<pair<vector<int>,int>> regions;
};

int point_size;
int region_size;
int temp;
int max_points;
int max_regions;
map<PageHandler,int> occupancy;
map<int,PageHandler> pages;
int num_pages;
int d;

int store(struct Node node, PageHandler &page, int offset, bool flag){
	// write the node to the page
	char *data = page.GetData ();
	if(flag){
		memcpy (&data[offset], &node.page_identifier, sizeof(int));
		memcpy (&data[offset+4], &node.node_type, sizeof(int));
		memcpy (&data[offset+8], &node.split_dim, sizeof(int));
		offset += 12;
	}

	if(node.node_type==0){
		for(int i=0;i<d;i++){
			memcpy (&data[offset], &node.regions[i], sizeof(int));
			offset+=4;
		}
		int location;
		memcpy (&data[offset], &location, sizeof(int));
		offset+=4;
	}else {
		for(int i=0;i<2*d;i++){
			memcpy (&data[offset], &node.regions[i], sizeof(int));
			offset+=4;
		}
		int location;
		memcpy (&data[offset], &location, sizeof(int));
		offset+=4;
	}

	return offset;

}

void insert(vector<int> &point, PageHandler &root, fstream &fsout){
	if(occupancy[root]==0){
		pages[num_pages++] = root;
		struct Node node;
		node.page_identifier = 0;
		node.node_type = 0;
		node.split_dim = 0;
		node.regions.push_back({point,0});
		store(node,root,0,true);

	}else {

	}
}

// void pquery(vector<int> point, fstream &fsout){

// }

// void rquery(vector<int> point_min, vector<int> point_max, fstream &fsout){

// }

// void reorganize(node Node){

// }

// void nodeSplit(node Node){

// }






int main(int argc, char* argv[]) {

    const char* query_file = argv[1];
    d = std::stoi(argv[2]);
    const char* output_file = argv[2];

	fstream fsin;
	fsin.open(query_file,ios::in);

	fstream fsout;
	fsout.open(output_file,ios::out);

	point_size = (d+1)*4;
	region_size = (2*d+1)*4;
	temp = PAGE_CONTENT_SIZE - 2*sizeof(int);
	max_points = temp/point_size;
	max_regions = temp/region_size;
	num_pages = 0;

	FileManager fm;
	FileHandler fh = fm.CreateFile("f1.txt");
	PageHandler root =  fh.NewPage();
	occupancy[root] = 0;
	


	// INSERT -1 -85
	// INSERT 56 37
	// INSERT 48 -25
	// INSERT -78 -29
	// INSERT 62 18

	// vector<int> p1 = {56,37};
	// insert(p1,)
	

	// if (fsin.is_open()){ 
	// 	string tp;
	// 	while(getline(fsin, tp)){
	// 		cout << tp << "\n";

	// 		istringstream ss(tp);
    // 		string word;
	// 		vector<string> temp;
    // 		while (ss >> word) 
    // 		{
    //     		temp.push_back(word);
    // 		}

	// 		if(tp[0]=='I'){ 		//INSERT
	// 			vector<int> point;
	// 			for(int i=0;i<d;i++){
	// 				point.push_back(stoi(temp[i+1]));
	// 			}

	// 			// Call insert

	// 		}
	// 		else if(tp[0]=='P'){	//PQUERY
	// 			vector<int> point;
	// 			for(int i=0;i<d;i++){
	// 				point.push_back(stoi(temp[i+1]));
	// 			}

	// 			// Call pquery
	// 		}
	// 		else{					//RQUERY
	// 			vector<int> point_min,point_max;
	// 			for(int i=0;i<2*d;i+=2){
	// 				point_min.push_back(stoi(temp[i+1]));
	// 				point_max.push_back(stoi(temp[i+2]));
	// 			}

	// 			// Call rquery
	// 		}
	// 	}
	// 	fsin.close();
	// 	fsout.close();
   	// }


}