#include <iostream>
#include <opencv2/opencv.hpp>

int main() {

    cv::Mat original =
        cv::imread("COMP_IMG/PTI1 CP.jpg");

    cv::Mat reconstructed =
        cv::imread("OUT/PTI1 CP.png");

    if (
        original.empty() ||
        reconstructed.empty()
    ) {
        std::cout << "Failed loading images\n";
        return 0;
    }

    if (
        original.rows != reconstructed.rows ||
        original.cols != reconstructed.cols
    ) {

        std::cout
            << "Dimension mismatch\n";

        return 0;
    }

    cv::Mat diff;

    cv::absdiff(
        original,
        reconstructed,
        diff
    );

    int non_zero =
        cv::countNonZero(
            diff.reshape(1)
        );

    if (non_zero == 0) {

        std::cout
            << "PIXEL PERFECT MATCH\n";
    }
    else {

        std::cout
            << "Pixel differences: "
            << non_zero
            << "\n";
    }

    return 0;
}