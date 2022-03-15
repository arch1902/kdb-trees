#include <iostream>
#include "file_manager.h"
#include "errors.h"
#include <cstring>
#include <vector>
#include <fstream>
#include <map>

using namespace std;

struct Node{
	int node_type; // 0 for point page, 1 for region page
	int split_dim; 
	int page_identifier;
    vector<pair<vector<int>,int>> regions; //points in case of point node
};

bool operator<(const PageHandler &l1, const PageHandler &l2)
        {
                return false;
        }

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
		for(auto region : node.regions){
			for(int i=0;i<d;i++){
				memcpy (&data[offset], &region.first[i], sizeof(int));
				offset+=4;
			}
			memcpy (&data[offset], &region.second, sizeof(int));
			offset+=4;
		}
	}else {
		for(auto region : node.regions){
			for(int i=0;i<2*d;i++){
				memcpy (&data[offset], &region.first[i], sizeof(int));
				offset+=4;
			}
			memcpy (&data[offset], &region.second, sizeof(int));
			offset+=4;
		}
	}
	return offset;
}

int addPoint(vector<int> &point, PageHandler &page, int offset){
	char *data = page.GetData ();
	for(int i=0;i<d;i++){
		memcpy (&data[offset], &point[i], sizeof(int));
		offset+=4;
	}
	int location;
	memcpy (&data[offset], &location, sizeof(int));
	offset+=4;
	occupancy[page]++;
	return offset;
}

// load function 

struct Node load(PageHandler &page){
	struct Node node;
	char *data = page.GetData ();
	memcpy (&node.page_identifier, &data[0], sizeof(int));
	memcpy (&node.node_type, &data[4], sizeof(int));
	memcpy (&node.split_dim, &data[8], sizeof(int));
	int offset = 12;
	if(node.node_type==0){
		vector<pair<vector<int>,int>> points;
		for(int i=0;i<occupancy[page];i++){
			vector<int> point(d,0);
			for(int j=0;j<d;j++){
				memcpy (&point[j], &data[offset], sizeof(int));
				offset+=4;
			}
			int location;
			memcpy (&location, &data[offset], sizeof(int));
			offset+=4;
			points.push_back({point,location});
		}
		node.regions = points;

	}else {
		vector<pair<vector<int>,int>> regions;
		for(int i=0;i<occupancy[page];i++){
			vector<int> region(2*d,0);
			for(int j=0;j<2*d;j++){
				memcpy (&region[j], &data[offset], sizeof(int));
				offset+=4;
			}
			int cid;
			memcpy (&cid, &data[offset], sizeof(int));
			offset+=4;
			regions.push_back({region,cid});
		}
		node.regions = regions;
	}

	return node;
}

void insert(vector<int> &point, PageHandler &root){
	if(occupancy[root]==0){
		pages[num_pages++] = root;
		struct Node node;
		node.page_identifier = 0;
		node.node_type = 0;
		node.split_dim = 0;
		node.regions.push_back({point,0});
		occupancy[root]+=1;
		int offset = store(node,root,0,true);

	}else {
		struct Node node = load(root);
		if(node.node_type==0){
			if(occupancy[root]<max_points){
				addPoint(point,root,occupancy[root]*point_size + 12);
			}
			// else 
			// {
			// 	// handle overflow
			// }
		}else {
			for(auto region : node.regions){
				bool flag = true;
				for(int i=0;i<d;i++){
					if(region.first[i]>point[i] or region.first[d+i]<=point[i]){
						flag = false;
						break;
					}
				}
				if(flag) {
					insert(point,pages[region.second]);
				}
			}
		}
	}
}



// void reorganize(node Node){

// }

// void nodeSplit(node Node){

// }

bool pquery(vector<int>& point, PageHandler &page){
	struct Node node = load(page);
	if(node.node_type==0){
		for(auto p: node.regions){
			bool flag = true;
			for(int i=0;i<d;i++){
				if(p.first[i]!=point[i]){
					flag = false;
					break;
				}
			}
			if(flag) return true;
		}
		return false;
	}else {
		for(auto region : node.regions){
			bool flag = true;
			for(int i=0;i<d;i++){
				if(region.first[i]>point[i] or region.first[d+i]<=point[i]){
					flag = false;
					break;
				}
			}
			if(flag) {
				pquery(point,pages[region.second]);
			}
		}
	}

}

// void rquery(vector<int> point_min, vector<int> point_max, fstream &fsout){

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
	FileHandler fh = fm.CreateFile("temp.txt");
	PageHandler root =  fh.NewPage();
	occupancy[root] = 0;
	


	// INSERT -1 -85
	// INSERT 56 37
	// INSERT 48 -25
	// INSERT -78 -29
	// INSERT 62 18

	vector<int> p1 = {56,37};
	vector<int> p2 = {56,38};
	insert(p1,root);
	insert(p2,root);
	cout<< pquery(p1,root)<<endl;
	cout<< pquery(p2,root)<<endl;
	

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

	fm.CloseFile (fh);
	fm.DestroyFile ("temp.txt");


}