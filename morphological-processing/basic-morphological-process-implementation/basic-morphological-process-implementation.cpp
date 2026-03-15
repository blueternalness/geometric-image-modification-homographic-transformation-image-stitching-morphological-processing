#include <iostream>
#include <vector>
#include <string>
#include <fstream>

using namespace std;

vector<unsigned char> readRaw(const string& filename, int w, int h) {
    vector<unsigned char> img(w * h, 0);
    ifstream file(filename, ios::binary);
    file.read(reinterpret_cast<char*>(img.data()), w * h);
    return img;
}

void writeRaw(const string& filename, const vector<unsigned char>& img, int w, int h) {
    vector<unsigned char> out(w * h);
    for (int i = 0; i < w * h; i++) {
        out[i] = img[i] * 255;
    }
    ofstream file(filename, ios::binary);
    file.write(reinterpret_cast<const char*>(out.data()), w * h);
}

void binarize(vector<unsigned char>& img) {
    for (size_t i = 0; i < img.size(); i++) {
        img[i] = (img[i] > 127) ? 1 : 0;
    }
}
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
        for (int y = 1; y < h - 1; y++) {
            for (int x = 1; x < w - 1; x++) {
                if (img[y * w + x] == 1) {
                    vector<int> p = {
                        img[(y - 1) * w + x],
                        img[(y - 1) * w + x + 1],
                        img[y * w + x + 1],
                        img[(y + 1) * w + x + 1],
                        img[(y + 1) * w + x],
                        img[(y + 1) * w + x - 1],
                        img[y * w + x - 1],
                        img[(y - 1) * w + x - 1]
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
        for (int i = 0; i < w * h; i++) {
            if (marker[i]) img[i] = 0;
        }
        fill(marker.begin(), marker.end(), 0);
        for (int y = 1; y < h - 1; y++) {
            for (int x = 1; x < w - 1; x++) {
                if (img[y * w + x] == 1) {
                    vector<int> p = {
                        img[(y - 1) * w + x],
                        img[(y - 1) * w + x + 1],
                        img[y * w + x + 1],
                        img[(y + 1) * w + x + 1],
                        img[(y + 1) * w + x],
                        img[(y + 1) * w + x - 1],
                        img[y * w + x - 1],
                        img[(y - 1) * w + x - 1]
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
        for (int i = 0; i < w * h; i++){
            if (marker[i]) img[i] = 0;
        }
        iter++;
        if (iter == 20) writeRaw(name + "_iter20.raw", img, w, h);
    }
    writeRaw(name + "_final.raw", img, w, h);
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