#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <string>

using namespace std;

struct Image {
    int width;
    int height;
    vector<unsigned char> data;
    Image(int w, int h) : width(w), height(h), data(w * h * 3, 0) {}
};

void readRaw(const string& filename, Image& img) {
    ifstream file(filename, ios::binary);
    file.read(reinterpret_cast<char*>(img.data.data()), img.data.size());
    file.close();
}

void writeRaw(const string& filename, const Image& img) {
    ofstream file(filename, ios::binary);
    file.write(reinterpret_cast<const char*>(img.data.data()), img.data.size());
    file.close();
}

vector<unsigned char> getPixelBilinear(const Image& img, double x, double y) {
    int x1 = floor(x);
    int y1 = floor(y);
    int x2 = min(x1 + 1, img.width - 1);
    int y2 = min(y1 + 1, img.height - 1);
    x1 = max(0, x1);
    y1 = max(0, y1);
    double dx = x - x1;
    double dy = y - y1;

    vector<unsigned char> color(3, 0);
    for (int c = 0; c < 3; ++c) {
        double p1 = img.data[(y1 * img.width + x1) * 3 + c];
        double p2 = img.data[(y1 * img.width + x2) * 3 + c];
        double p3 = img.data[(y2 * img.width + x1) * 3 + c];
        double p4 = img.data[(y2 * img.width + x2) * 3 + c];
        double val = p1 * (1 - dx) * (1 - dy) + p2 * dx * (1 - dy) + p3 * (1 - dx) * dy + p4 * dx * dy;
        color[c] = static_cast<unsigned char>(clamp(val, 0.0, 255.0));
    }
    return color;
}

pair<double, double> forward_map(double x, double y, double k) {
    double xp = x, yp = y;
    if (abs(x) >= abs(y)) {
        if (x != 0) xp = x * (1.0 - k) + k * (y * y) / x;
    } else {
        if (y != 0) yp = y * (1.0 - k) + k * (x * x) / y;
    }
    return {xp, yp};
}

pair<double, double> inverse_map(double xp, double yp, double k) {
    double x = xp, y = yp;
    if (abs(xp) >= abs(yp)) {
        double discriminant = xp * xp - 4.0 * k * (1.0 - k) * yp * yp;
        if (discriminant < 0) return {1000.0, 1000.0};
        double sign_xp = (xp >= 0) ? 1.0 : -1.0;
        x = (xp + sign_xp * sqrt(discriminant)) / (2.0 * (1.0 - k));
    } else {
        double discriminant = yp * yp - 4.0 * k * (1.0 - k) * xp * xp;
        if (discriminant < 0) return {1000.0, 1000.0}; 
        double sign_yp = (yp >= 0) ? 1.0 : -1.0;
        y = (yp + sign_yp * sqrt(discriminant)) / (2.0 * (1.0 - k));
    }
    return {x, y};
}

Image warpImage(const Image& src) {
    Image dst(src.width, src.height);
    double cx = src.width / 2.0;
    double cy = src.height / 2.0;
    double k = 128.0 / cx;

    for (int r = 0; r < dst.height; ++r) {
        for (int c = 0; c < dst.width; ++c) {
            double xp = c - cx;
            double yp = cy - r;
            pair<double, double> src_coords = inverse_map(xp, yp, k);
            double x = src_coords.first;
            double y = src_coords.second;

            if (abs(x) > cx || abs(y) > cy) {
                int idx = (r * dst.width + c) * 3;
                dst.data[idx] = dst.data[idx+1] = dst.data[idx+2] = 0;
            } else {
                double src_c = x + cx;
                double src_r = cy - y;
                vector<unsigned char> color = getPixelBilinear(src, src_c, src_r);
                int idx = (r * dst.width + c) * 3;
                dst.data[idx]   = color[0];
                dst.data[idx+1] = color[1];
                dst.data[idx+2] = color[2];
            }
        }
    }
    return dst;
}

Image reverseWarpImage(const Image& warped) {
    Image recovered(warped.width, warped.height);
    double cx = warped.width / 2.0;
    double cy = warped.height / 2.0;
    double k = 128.0 / cx; 

    for (int r = 0; r < recovered.height; ++r) {
        for (int c = 0; c < recovered.width; ++c) {
            double x = c - cx;
            double y = cy - r;
            pair<double, double> warped_coords = forward_map(x, y, k);
            double xp = warped_coords.first;
            double yp = warped_coords.second;
            double src_c = xp + cx;
            double src_r = cy - yp;
            
            vector<unsigned char> color = getPixelBilinear(warped, src_c, src_r);
            int idx = (r * recovered.width + c) * 3;
            recovered.data[idx]   = color[0];
            recovered.data[idx+1] = color[1];
            recovered.data[idx+2] = color[2];
        }
    }
    return recovered;
}

int main() {
    int w = 800, h = 800;
    vector<string> names = {"Bird", "Cat"};

    for (const string& name : names) {
        Image original(w, h);
        readRaw(name + ".raw", original);

        Image warped = warpImage(original);
        writeRaw(name + "_Warped.raw", warped);

        Image recovered = reverseWarpImage(warped);
        writeRaw(name + "_Recovered.raw", recovered);
    }
    return 0;
}
