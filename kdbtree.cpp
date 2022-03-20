#include <iostream>
#include "file_manager.h"
#include "errors.h"
#include <cstring>
#include <vector>
#include <fstream>
#include <map>
#include <algorithm>
#include <cmath>
#include <queue>
using namespace std;

int INF = 1000000000;

struct Node{
	int page_identifier;
	int node_type; // 0 for point page, 1 for region page
	int split_dim; 
	int parent;
    vector<pair<vector<int>,int>> regions; //points in case of point node
};

int median(vector<int> v){
    int n = v.size();
   sort(v.begin(), v.end());
   if (n%2!= 0)
      return ceil((double)v[n/2]);
   return ceil((double)(v[(n-1)/2] + v[n/2])/2.0);
}

// bool operator<(const PageHandler &l1, const PageHandler &l2)
//         {
//                 return false;
//         }

int point_size;
int region_size;
int temp;
int max_points;
int max_regions;
int occupancy[10000009]; // no. of integers stores in the page
// map<int,PageHandler> pages; // page_identifier to PageHandler mapping
int num_pages; // current number of pages in buffer
int d; // dimension
FileManager fm;
FileHandler fh;
PageHandler root;

int store(struct Node node, PageHandler &page, int offset, bool flag){
	// write the node to the page
	char *data = page.GetData ();
	if(flag){
		memcpy (&data[offset], &node.page_identifier, sizeof(int));
		memcpy (&data[offset+4], &node.node_type, sizeof(int));
		memcpy (&data[offset+8], &node.split_dim, sizeof(int));
		memcpy (&data[offset+12], &node.parent, sizeof(int));
		offset += 16;
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
	int location=0;
	memcpy (&data[offset], &location, sizeof(int));
	offset+=4;
	int id;
	memcpy (&id, &data[0], sizeof(int));
	occupancy[id]++;
	return offset;
}

// load function 

struct Node load(PageHandler &page){
	struct Node node;
	char *data = page.GetData ();
	memcpy (&node.page_identifier, &data[0], sizeof(int));
	memcpy (&node.node_type, &data[4], sizeof(int));
	memcpy (&node.split_dim, &data[8], sizeof(int));
	memcpy (&node.parent, &data[12], sizeof(int));
	int offset = 16;
	int id;
	memcpy (&id, &data[0], sizeof(int));
	if(node.node_type==0){
		vector<pair<vector<int>,int>> points;
		for(int i=0;i<occupancy[id];i++){
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
		for(int i=0;i<occupancy[id];i++){
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

void reorganize(Node &node, PageHandler &page);

void insert(vector<int> &point, PageHandler &root){
	if(occupancy[root.GetPageNum()]==0){
		// pages[num_pages++] = root;
		struct Node node;
		node.page_identifier = root.GetPageNum();
		node.node_type = 0;
		node.split_dim = 0;
		node.parent = -1;
		node.regions.push_back({point,0});
		occupancy[root.GetPageNum()]+=1;
		int offset = store(node,root,0,true);
		cout<<"INSERTION DONE:"<<endl;
		for(auto x:point) cout<<x<<" ";
		cout<<endl<<endl<<endl;

	}else {
		struct Node node = load(root);
		if(node.node_type==0){
			if(occupancy[root.GetPageNum()]<max_points){
				node.regions.push_back({point,0});
				int t = addPoint(point,root,occupancy[root.GetPageNum()]*point_size + 16);
				//cout<<"Added "<<point[0]<<" "<<point[1]<<" at "<<t<<endl;
				cout<<"INSERTION DONE:"<<endl;
				for(auto point: node.regions){
					for(auto x:point.first) cout<<x<<" ";
					cout<<endl;
				}
				cout<<endl<<endl;
			}
			else 
			{
				node.regions.push_back({point,0});
				//cout<<"oops Overflow! "<< node.regions.size() <<endl;
				reorganize(node,root);
			}
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
					PageHandler temp = fh.PageAt(region.second);
					insert(point,temp);
				}
			}
		}
	}
}

void splitPointNode(Node &node,int split_elem, Node &left, Node &right){
	int split_dim = node.split_dim;

	left.split_dim = (split_dim + 1)%d;
	left.node_type = 0;
	PageHandler leftPage = fh.NewPage();
	left.page_identifier = leftPage.GetPageNum();
	left.parent = node.page_identifier;
	occupancy[leftPage.GetPageNum()]=0;

	right.split_dim = (split_dim + 1)%d;
	right.node_type = 0;
	PageHandler rightPage = fh.NewPage();
	right.page_identifier = rightPage.GetPageNum();
	right.parent = node.page_identifier;
	occupancy[rightPage.GetPageNum()]=0;

	int side = -1; 
	for(auto point : node.regions){
		if(point.first[split_dim]<split_elem){
			left.regions.push_back(point);
			occupancy[leftPage.GetPageNum()]++;
			if(point==node.regions.back()) side=0;
			//cout<<"Left Inserted: "<<point.first[0]<<" "<<point.first[1]<<endl;
		}else {
			right.regions.push_back(point);
			occupancy[rightPage.GetPageNum()]++;
			if(point==node.regions.back()) side=1;
			//cout<<"Right Inserted: "<<point.first[0]<<" "<<point.first[1]<<endl;
		}
	}
	cout<<"INSERTION DONE:"<<endl;
	vector<int> v;
	if(side==0){
		for(auto point: left.regions){
			for(auto x:point.first) cout<<x<<" ";
			cout<<endl;
		}
		cout<<endl<<endl;
	}else {
		for(auto point: right.regions){
			for(auto x:point.first) cout<<x<<" ";
			cout<<endl;
		}
		cout<<endl<<endl;
	}

	store(left,leftPage,0,true);
	store(right,rightPage,0,true);


}



void reorganize(Node &node, PageHandler &page){
	if(node.node_type == 0 and node.parent==-1){
		int split_dim = node.split_dim;
		vector<int> v;
		for(auto point : node.regions){
			v.push_back(point.first[split_dim]);
		}
		int split_elem = median(v);
		Node left,right;

		splitPointNode(node,split_elem,left,right);

		node.node_type = 1;
		node.regions.clear();

		vector<int> rl(2*d);
		vector<int> rr(2*d);

		for(int i=0;i<d;i++){
			if(i==split_dim){
				rl[i] = -INF;
				rl[d+i] = split_elem;
				rr[i] = split_elem;
				rr[d+i] = INF;

			}else {
				rl[i] = -INF;
				rl[d+i] = INF;
				rr[i] = -INF;
				rr[d+i] = INF;
			}
		}
		node.regions.push_back({rl,left.page_identifier});
		//cout<<"left page : "<<left.page_identifier<<endl;
		node.regions.push_back({rr,right.page_identifier});
		//cout<<"right page : "<<right.page_identifier<<endl;
		occupancy[page.GetPageNum()] = 2;
		store(node,page,0,true);
	}
	else if (node.node_type==0){
		int split_dim = node.split_dim;
		vector<int> v;
		for(auto point : node.regions){
			v.push_back(point.first[split_dim]);
		}
		int split_elem = median(v);
		Node left,right;

		splitPointNode(node,split_elem,left,right);

		PageHandler parentPage = fh.PageAt(node.parent);
		Node parentNode = load(parentPage);

		for(int i=0;i<occupancy[parentPage.GetPageNum()];i++){
			if(parentNode.regions[i].second==node.page_identifier){
				vector<int> rl(2*d);
				vector<int> rr(2*d);
				for(int j=0;j<d;j++){
					if(j==split_dim){
						rl[j+d] = split_elem;
						rl[j] = parentNode.regions[i].first[j];
						rr[j] = split_elem;
						rr[j+d] = parentNode.regions[i].first[j+d];
					}else {
						rl[j+d] = parentNode.regions[i].first[j+d];
						rl[j] = parentNode.regions[i].first[j];
						rr[j] = parentNode.regions[i].first[j];
						rr[j+d] = parentNode.regions[i].first[j+d];
					}
				}
				parentNode.regions.push_back({rl,left.page_identifier});
				parentNode.regions.push_back({rr,right.page_identifier});
				parentNode.regions.erase(parentNode.regions.begin() + i);
				break;
			}
		}

		if(occupancy[parentPage.GetPageNum()]<max_regions){
			occupancy[parentPage.GetPageNum()]++;
			store(parentNode,parentPage,0,true);
		}else {
			reorganize(parentNode,parentPage);
		}
	}
	// else if(node.node_type==1 and node.parent!=-1){

	// }
	// else{

	// }
}

// void nodeSplit(node Node){

// }

int pquery(vector<int>& point, PageHandler &page, int depth=0){
	struct Node node = load(page);
	if(node.node_type==0){
		//  for(auto p: node.regions)
		// 	cout<<p.first[0]<<" "<<p.first[1]<<endl;
		for(auto p: node.regions){
			bool flag = true;
			for(int i=0;i<d;i++){
				if(p.first[i]!=point[i]){
					flag = false;
					break;
				}
			}
			//cout<<"Flag = "<<flag<<endl;
			if(flag) {
				cout<<"NUM REGION NODES TOUCHED: "<<depth<<endl;
				cout<<"TRUE"<<endl<<endl<<endl;
				return flag;
			}
		}
		cout<<"NUM REGION NODES TOUCHED: "<<0<<endl;
		cout<<"FALSE"<<endl<<endl<<endl;
		return 0;

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
				PageHandler temp = fh.PageAt(region.second);
				//cout<<"Region chosen:"<<region.second<<endl;
				return pquery(point,temp,depth+1);
			}
		}
	}

}

void rquery(vector<int> point_min, vector<int> point_max, PageHandler &root, fstream &fsout){
	vector<vector<int>> result;
	if(occupancy[root.GetPageNum()]==0){
		fsout<<"NO POINT FOUND"<<endl;
	}
	queue<pair<int,int>> q;
	q.push({root.GetPageNum(),0});
	while(!q.empty()){
		PageHandler currPage = fh.PageAt(q.front().first);
		int depth = q.front().second;
		q.pop();
		Node node = load(currPage);
		if(node.node_type==0){
			for(auto point: node.regions){
				bool flag = true;
				for(int i=0;i<d;i++){
					if(point.first[i]<point_min[i] or point.first[i]>=point_max[i]){
						flag = false;
						break;
					}
				}
				point.first.push_back(depth);
				if(flag) result.push_back(point.first);
			}
		}else {
			for(auto region: node.regions){
				bool flag = true;
				for(int i=0;i<d;i++){
					if(region.first[i]>=point_max[i] or region.first[i+d]<point_min[i]){
						flag = false;
						break;
					}
				}
				if(flag) q.push({region.second,depth+1});
			}
		}
	}
	if(result.size()==0){
		cout<<"NO POINT FOUND"<<endl<<endl<<endl;
		return;
	}

	for(int i=0;i<result.size();i++){
		cout<<"POINT: ";
		for(int j=0;j<d;j++) cout<<result[i][j]<<" ";
		cout<<"NUM REGION NODES TOUCHED: "<<result[i][d];
		cout<<endl;
	}
	cout<<endl<<endl;
}


int main(int argc, char* argv[]) {

    const char* query_file = argv[1];
    d = std::stoi(argv[2]);
    const char* output_file = argv[2];
	const char* buffer_log = argv[3];

	fstream fsin;
	fsin.open(query_file,ios::in);

	fstream fsout;
	fsout.open(output_file,ios::out);

	point_size = (d+1)*4;
	region_size = (2*d+1)*4;
	temp = PAGE_CONTENT_SIZE - 3*sizeof(int);
	max_points = temp/point_size;
	max_regions = temp/region_size;
	num_pages = 0;

	cout<<"Max points:"<<max_points<<endl;
	cout<<"Max regions:"<<max_regions<<endl;

	
	fh = fm.CreateFile("temp.txt");
	root =  fh.NewPage();
	occupancy[root.GetPageNum()] = 0;
	

	vector<int> p1 = {2,3};
	vector<int> p2 = {5,4};
	vector<int> p3 = {9,6};
	vector<int> p4 = {4,7};
	vector<int> p5 = {8,1};
	vector<int> p6 = {7,2};
	vector<int> p7 = {6,3};
	vector<int> p8 = {10,11};
	vector<int> p9 = {12,11};
	vector<int> p10 = {5,10};
	vector<int> rmin = {100,1};
	vector<int> rmax = {11,20};


	insert(p1,root);
	insert(p2,root);
	insert(p3,root);
	insert(p4,root);
	insert(p5,root);
	insert(p6,root);
	insert(p7,root);
	insert(p8,root);
	insert(p9,root);


	pquery(p1,root);
	pquery(p2,root);
	pquery(p3,root);
	pquery(p4,root);
	pquery(p5,root);
	pquery(p6,root);
	pquery(p7,root);
	pquery(p8,root);
	pquery(p9,root);
	pquery(p10,root);

	rquery(rmin,rmax,root,fsout);

	

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