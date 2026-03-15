#include <iostream>
#include <fstream>
#include <vector>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

// 1. Read a raw 24-bit RGB image
Mat readRawRGB(const string& filename, int width, int height) {
    ifstream file(filename, ios::binary);
    if (!file) {
        cerr << "Error opening file: " << filename << endl;
        return Mat();
    }
    
    Mat img(height, width, CV_8UC3);
    file.read(reinterpret_cast<char*>(img.data), width * height * 3);
    file.close();
    
    // Convert RGB to BGR for OpenCV processing
    cvtColor(img, img, COLOR_RGB2BGR); 
    return img;
}

// 2. Write a raw 24-bit RGB image
void writeRawRGB(const string& filename, const Mat& img) {
    // Convert OpenCV's BGR back to standard RGB
    Mat outputRGB;
    cvtColor(img, outputRGB, COLOR_BGR2RGB);
    
    ofstream outFile(filename, ios::binary);
    if (!outFile.is_open()) {
        cerr << "Error creating output file: " << filename << endl;
        return;
    }
    
    outFile.write(reinterpret_cast<const char*>(outputRGB.data), outputRGB.total() * outputRGB.elemSize());
    outFile.close();
}

// 3. Manual 8-Parameter Homography Solver (Ah = B using 4 point pairs)
Mat computeHomographyManual(const vector<Point2f>& src, const vector<Point2f>& dst) {
    CV_Assert(src.size() == 4 && dst.size() == 4);
    
    Mat A(8, 8, CV_64F, Scalar(0));
    Mat B(8, 1, CV_64F, Scalar(0));
    
    for (int i = 0; i < 4; i++) {
        double x1 = src[i].x, y1 = src[i].y;
        double x2 = dst[i].x, y2 = dst[i].y;
        
        A.at<double>(i*2, 0) = x1;
        A.at<double>(i*2, 1) = y1;
        A.at<double>(i*2, 2) = 1;
        A.at<double>(i*2, 6) = -x1 * x2;
        A.at<double>(i*2, 7) = -y1 * x2;
        B.at<double>(i*2, 0) = x2;
        
        A.at<double>(i*2+1, 3) = x1;
        A.at<double>(i*2+1, 4) = y1;
        A.at<double>(i*2+1, 5) = 1;
        A.at<double>(i*2+1, 6) = -x1 * y2;
        A.at<double>(i*2+1, 7) = -y1 * y2;
        B.at<double>(i*2+1, 0) = y2;
    }
    
    Mat h;
    solve(A, B, h, DECOMP_SVD);
    
    Mat H(3, 3, CV_64F);
    for (int i = 0; i < 8; i++) {
        H.at<double>(i / 3, i % 3) = h.at<double>(i, 0);
    }
    H.at<double>(2, 2) = 1.0; 
    
    return H;
}

// 4. Feature matching and Homography estimation using SIFT
Mat getHomography(const Mat& img1, const Mat& img2) {
    Ptr<SIFT> sift = SIFT::create();
    vector<KeyPoint> kp1, kp2;
    Mat desc1, desc2;
    
    sift->detectAndCompute(img1, noArray(), kp1, desc1);
    sift->detectAndCompute(img2, noArray(), kp2, desc2);
    
    BFMatcher matcher(NORM_L2);
    vector<vector<DMatch>> knn_matches;
    matcher.knnMatch(desc1, desc2, knn_matches, 2);
    
    // Lowe's Ratio Test
    vector<DMatch> good_matches;
    for (size_t i = 0; i < knn_matches.size(); i++) {
        if (knn_matches[i][0].distance < 0.75f * knn_matches[i][1].distance) {
            good_matches.push_back(knn_matches[i][0]);
        }
    }
    
    vector<Point2f> pts1, pts2;
    for (auto& m : good_matches) {
        pts1.push_back(kp1[m.queryIdx].pt);
        pts2.push_back(kp2[m.trainIdx].pt);
    }

    cout << "\\begin{itemize}" << endl;
    // Print the first 4 good matches
    for (int i = 0; i < 4 && i < pts1.size(); ++i) {
        cout << "    \\item Point " << i + 1 << ": Image 1 $(" 
             << round(pts1[i].x) << ", " << round(pts1[i].y) 
             << ") \\rightarrow$ Image 2 $(" 
             << round(pts2[i].x) << ", " << round(pts2[i].y) << ")" << endl;
    }
    cout << "\\end{itemize}" << endl;    

// ... inside getHomography(), right after finding good_matches ...

    // Ensure we have at least 4 matches, then take exactly the first 4
    if (good_matches.size() >= 4) {
        vector<DMatch> matches_to_draw(good_matches.begin(), good_matches.begin() + 4);

        Mat img_matches;
        // drawMatches creates a side-by-side image with lines connecting the 4 points
        drawMatches(img1, kp1, img2, kp2, matches_to_draw, img_matches, 
                    Scalar::all(-1), Scalar::all(-1), vector<char>(), 
                    DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);

        // Save the visualization
        static int match_counter = 1;
        string filename = "matches_pair_4pts_" + to_string(match_counter++) + ".png";
        imwrite(filename, img_matches);
        
        cout << "Saved visualization with exactly 4 matches to: " << filename << endl;
    } else {
        cerr << "Error: Not enough good matches found to draw 4 points." << endl;
    }

    // Using RANSAC to find the best homography matrix
    return findHomography(pts1, pts2, RANSAC);
}

int main() {
    int width = 600, height = 400;
    
    // Read the source images
    Mat imgLeft = readRawRGB("Broad_left.raw", width, height);
    Mat imgMid = readRawRGB("Broad_middle.raw", width, height);
    Mat imgRight = readRawRGB("Broad_right.raw", width, height);
    
    if (imgLeft.empty() || imgMid.empty() || imgRight.empty()) {
        cerr << "Failed to load one or more images." << endl;
        return -1;
    }
    
    // Compute Homographies targeting the Middle image
    Mat H_left_to_mid = getHomography(imgLeft, imgMid);
    Mat H_right_to_mid = getHomography(imgRight, imgMid);
    
    // Setup Panorama Canvas
    int panoWidth = width * 3;
    int panoHeight = static_cast<int>(height * 1.5);
    Mat panorama = Mat::zeros(panoHeight, panoWidth, CV_8UC3);
    
    // Shift the origin to center the middle image on the canvas
    Mat translation = (Mat_<double>(3, 3) << 1, 0, width,  0, 1, height * 0.25,  0, 0, 1);
    
    Mat H_left_final = translation * H_left_to_mid;
    Mat H_right_final = translation * H_right_to_mid;
    
    // Warp Images
    Mat warpedLeft, warpedRight, warpedMid;
    warpPerspective(imgLeft, warpedLeft, H_left_final, Size(panoWidth, panoHeight));
    warpPerspective(imgRight, warpedRight, H_right_final, Size(panoWidth, panoHeight));
    warpPerspective(imgMid, warpedMid, translation, Size(panoWidth, panoHeight));
    
    // Blend the images
    for (int y = 0; y < panorama.rows; ++y) {
        for (int x = 0; x < panorama.cols; ++x) {
            Vec3b pL = warpedLeft.at<Vec3b>(y, x);
            Vec3b pM = warpedMid.at<Vec3b>(y, x);
            Vec3b pR = warpedRight.at<Vec3b>(y, x);
            
            if (pM != Vec3b(0,0,0)) panorama.at<Vec3b>(y, x) = pM;
            else if (pL != Vec3b(0,0,0)) panorama.at<Vec3b>(y, x) = pL;
            else if (pR != Vec3b(0,0,0)) panorama.at<Vec3b>(y, x) = pR;
        }
    }
    
    // Save as RAW file
    string outputFilename = "panorama_result.raw";
    writeRawRGB(outputFilename, panorama);
    
    // Output dimensions so the user knows how to view the raw file
    cout << "Panorama successfully written to: " << outputFilename << endl;
    cout << "Important: To view the RAW file, use the following dimensions:" << endl;
    cout << "Width: " << panoWidth << endl;
    cout << "Height: " << panoHeight << endl;
    cout << "Format: 24-bit RGB" << endl;
    
    return 0;
}