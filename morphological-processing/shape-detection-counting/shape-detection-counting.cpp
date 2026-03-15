#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <queue>
#include <algorithm>

using namespace std;

// --- Helper Functions ---
vector<unsigned char> readRaw(const string& filename, int w, int h) {
    vector<unsigned char> img(w * h, 0);
    ifstream file(filename, ios::binary);
    if (file) file.read(reinterpret_cast<char*>(img.data()), w * h);
    return img;
}

void binarize(vector<unsigned char>& img) {
    for (size_t i = 0; i < img.size(); i++) {
        img[i] = (img[i] > 127) ? 1 : 0;
    }
}

// --- Shape Detection Algorithms ---
void floodFillEdges(vector<unsigned char>& img, int w, int h) {
    queue<pair<int, int>> q;
    for (int x = 0; x < w; x++) {
        if (img[0 * w + x] == 1) q.push({x, 0});
        if (img[(h - 1) * w + x] == 1) q.push({x, h - 1});
    }
    for (int y = 0; y < h; y++) {
        if (img[y * w + 0] == 1) q.push({0, y});
        if (img[y * w + (w - 1)] == 1) q.push({w - 1, y});
    }
    
    while (!q.empty()) {
        auto [x, y] = q.front();
        q.pop();
        img[y * w + x] = 2; 
        
        int dx[] = {-1, 1, 0, 0};
        int dy[] = {0, 0, -1, 1};
        for (int i = 0; i < 4; i++) {
            int nx = x + dx[i], ny = y + dy[i];
            if (nx >= 0 && nx < w && ny >= 0 && ny < h && img[ny * w + nx] == 1) {
                img[ny * w + nx] = 2; 
                q.push({nx, ny});
            }
        }
    }
    for(int i = 0; i < w * h; i++) img[i] = (img[i] == 2) ? 1 : 0;
}

void countConnectedComponents(const vector<unsigned char>& img, int w, int h, vector<vector<pair<int,int>>>& blobs) {
    vector<bool> visited(w * h, false);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            if (img[y * w + x] == 1 && !visited[y * w + x]) {
                queue<pair<int, int>> q;
                vector<pair<int,int>> currentBlob;
                q.push({x, y});
                visited[y * w + x] = true;
                
                while (!q.empty()) {
                    auto [cx, cy] = q.front();
                    q.pop();
                    currentBlob.push_back({cx, cy});
                    for (int dy = -1; dy <= 1; dy++) {
                        for (int dx = -1; dx <= 1; dx++) {
                            int nx = cx + dx, ny = cy + dy;
                            if (nx >= 0 && nx < w && ny >= 0 && ny < h && img[ny * w + nx] == 1 && !visited[ny * w + nx]) {
                                visited[ny * w + nx] = true;
                                q.push({nx, ny});
                            }
                        }
                    }
                }
                blobs.push_back(currentBlob);
            }
        }
    }
}

void processBoard(const string& filename) {
    int w = 445, h = 445;
    vector<unsigned char> img = readRaw(filename, w, h);
    if (img.empty()) {
        cerr << "Failed to load image." << endl;
        return;
    }
    binarize(img);
    
    // 1. Find Holes
    vector<unsigned char> invImg(w * h);
    for (int i = 0; i < w * h; i++) invImg[i] = 1 - img[i];
    
    vector<unsigned char> background = invImg;
    floodFillEdges(background, w, h); 
    
    vector<unsigned char> holes(w * h);
    for (int i = 0; i < w * h; i++) holes[i] = invImg[i] - background[i];
    
    vector<vector<pair<int,int>>> holeBlobs;
    countConnectedComponents(holes, w, h, holeBlobs);
    int totalHoles = holeBlobs.size();
    
    // 2. Find Objects 
    vector<unsigned char> solidImg(w * h);
    for (int i = 0; i < w * h; i++) solidImg[i] = img[i] | holes[i];
    
    vector<vector<pair<int,int>>> objects;
    countConnectedComponents(solidImg, w, h, objects);
    int totalObjects = objects.size();
    
    // 3 & 4. Classify Rectangles and Circles using Extent
    int totalRectangles = 0;
    int totalCircles = 0;
    
    for (const auto& blob : objects) {
        // Find bounding box limits
        int minX = w, maxX = 0, minY = h, maxY = 0;
        for (auto p : blob) {
            minX = min(minX, p.first); maxX = max(maxX, p.first);
            minY = min(minY, p.second); maxY = max(maxY, p.second);
        }
        
        // Calculate areas
        double boundingBoxArea = (maxX - minX + 1) * (maxY - minY + 1);
        double objectArea = blob.size();
        
        // Calculate Extent metric
        double extent = objectArea / boundingBoxArea;
        
        // Axis-aligned rectangles fill ~100% of their bounding box (>0.90)
        // Circles fill roughly pi/4 (~78.5%) of their bounding box (<0.90)
        if (extent > 0.90) {
            totalRectangles++;
        } else {
            totalCircles++;
        }
    }
    
    cout << "\n--- Board Analysis Results ---" << endl;
    cout << "(1) Total Number of Holes: " << totalHoles << endl;
    cout << "(2) Total Number of White Objects: " << totalObjects << endl;
    cout << "(3) Total Number of White Rectangles: " << totalRectangles << endl;
    cout << "(4) Total Number of White Circles: " << totalCircles << endl;
}

int main() {
    processBoard("Board.raw");
    return 0;
}