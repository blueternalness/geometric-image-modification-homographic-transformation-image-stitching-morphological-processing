#include <iostream>
#include <vector>
#include <string>
#include <fstream>

using namespace std;

// --- Helper Functions ---

vector<unsigned char> readRaw(const string& filename, int w, int h) {
    vector<unsigned char> img(w * h, 0);
    ifstream file(filename, ios::binary);
    if (file) {
        file.read(reinterpret_cast<char*>(img.data()), w * h);
    } else {
        cerr << "Error: Could not open " << filename << endl;
    }
    return img;
}

void writeRaw(const string& filename, const vector<unsigned char>& img, int w, int h) {
    vector<unsigned char> out(w * h);
    // Convert 0/1 back to 0/255 for visibility
    for (int i = 0; i < w * h; i++) out[i] = img[i] * 255;
    ofstream file(filename, ios::binary);
    if (file) file.write(reinterpret_cast<const char*>(out.data()), w * h);
}

void binarize(vector<unsigned char>& img) {
    for (size_t i = 0; i < img.size(); i++) {
        img[i] = (img[i] > 127) ? 1 : 0; // 0.5 * 255
    }
}

// --- Thinning Algorithm ---

int getTransitions(const vector<int>& neighbors) {
    int count = 0;
    for (int i = 0; i < 8; i++) {
        if (neighbors[i] == 0 && neighbors[(i + 1) % 8] == 1) count++;
    }
    return count;
}

void zhangSuenThinning(vector<unsigned char>& img, int w, int h, const string& name) {
    bool imageChanged = true;
    int iter = 0;
    
    while (imageChanged) {
        imageChanged = false;
        vector<unsigned char> marker(w * h, 0);
        
        // Step 1
        for (int y = 1; y < h - 1; y++) {
            for (int x = 1; x < w - 1; x++) {
                if (img[y * w + x] == 1) {
                    vector<int> p = {
                        img[(y - 1) * w + x],     // P2 (up)
                        img[(y - 1) * w + x + 1], // P3
                        img[y * w + x + 1],       // P4 (right)
                        img[(y + 1) * w + x + 1], // P5
                        img[(y + 1) * w + x],     // P6 (down)
                        img[(y + 1) * w + x - 1], // P7
                        img[y * w + x - 1],       // P8 (left)
                        img[(y - 1) * w + x - 1]  // P9
                    };
                    int B = 0; for (int val : p) B += val;
                    int A = getTransitions(p);
                    if (B >= 2 && B <= 6 && A == 1 &&
                        (p[0] * p[2] * p[4] == 0) && (p[2] * p[4] * p[6] == 0)) {
                        marker[y * w + x] = 1;
                        imageChanged = true;
                    }
                }
            }
        }
        for (int i = 0; i < w * h; i++) if (marker[i]) img[i] = 0;
        
        // Step 2
        fill(marker.begin(), marker.end(), 0);
        for (int y = 1; y < h - 1; y++) {
            for (int x = 1; x < w - 1; x++) {
                if (img[y * w + x] == 1) {
                    vector<int> p = {
                        img[(y - 1) * w + x], img[(y - 1) * w + x + 1], img[y * w + x + 1],
                        img[(y + 1) * w + x + 1], img[(y + 1) * w + x], img[(y + 1) * w + x - 1],
                        img[y * w + x - 1], img[(y - 1) * w + x - 1]
                    };
                    int B = 0; for (int val : p) B += val;
                    int A = getTransitions(p);
                    if (B >= 2 && B <= 6 && A == 1 &&
                        (p[0] * p[2] * p[6] == 0) && (p[0] * p[4] * p[6] == 0)) {
                        marker[y * w + x] = 1;
                        imageChanged = true;
                    }
                }
            }
        }
        for (int i = 0; i < w * h; i++) if (marker[i]) img[i] = 0;
        
        iter++;
        // Save intermediate 20th iteration
        if (iter == 20) writeRaw(name + "_iter20.raw", img, w, h);
    }
    
    // Save Final Converged
    writeRaw(name + "_final.raw", img, w, h);
    cout << "Finished thinning " << name << " in " << iter << " iterations." << endl;
}

int main() {
    string files[] = {"Jar.raw", "Moon.raw", "Spring.raw", "Star.raw"};
    for (const string& f : files) {
        vector<unsigned char> img = readRaw(f, 512, 512);
        if(!img.empty()) {
            binarize(img);
            string baseName = f.substr(0, f.find("."));
            zhangSuenThinning(img, 512, 512, baseName);
        }
    }
    return 0;
}