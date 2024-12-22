#ifndef UTILITY_HPP
#define UTILITY_HPP

#include "boost/date_time/posix_time/posix_time.hpp"
#include "products.hpp"
#include <boost/date_time/gregorian/gregorian.hpp>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <time.h>

using boost::posix_time::microsec_clock;
using boost::posix_time::ptime;
using namespace std;
using namespace boost::gregorian;

/**
 * Updated bond information mapping:
 * Maps an integer maturity (2, 3, 5, 7, 10, 20, 30) to a pair consisting of
 * a CUSIP string and a boost::gregorian::date (representing maturity date).
 */
const map<int, pair<string, date>> bondMap({
    {2,  {"912828V23", date(2026, Dec, 15)}},
    {3,  {"912828W22", date(2027, Dec, 15)}},
    {5,  {"912828X21", date(2029, Dec, 15)}},
    {7,  {"912828Y20", date(2031, Dec, 15)}},
    {10, {"912828Z19", date(2034, Dec, 15)}},
    {20, {"912810FZ8", date(2044, Dec, 15)}},
    {30, {"912810GZ6", date(2054, Dec, 15)}}
});

/**
 * Associates each CUSIP string with an integer maturity in years.
 * E.g., "912828V23" -> 2.
 */
const map<string, int> bondId({
    {"912828V23", 2},
    {"912828W22", 3},
    {"912828X21", 5},
    {"912828Y20", 7},
    {"912828Z19", 10},
    {"912810FZ8", 20},
    {"912810GZ6", 30}
});

/**
 * Updated mapping of bond coupon rates, keyed by CUSIP.
 */
const map<string, double> bondCoupon({
    {"912828V23", 0.0425},
    {"912828W22", 0.0430},
    {"912828X21", 0.0435},
    {"912828Y20", 0.0440},
    {"912828Z19", 0.0445},
    {"912810FZ8", 0.0450},
    {"912810GZ6", 0.0455}
});

/**
 * Converts a fractional price string (e.g., "99-16+") into a decimal price.
 * The format is "basePrice-XYz" where:
 *   - basePrice: integer part
 *   - XY: two-digit fraction (0..31)
 *   - z: extra fraction in 256ths (0..7 or '+', which represents 4)
 */
double string2price(const std::string& fractional) {
    size_t dashPos = fractional.find('-');
    if (dashPos == std::string::npos) {
        throw std::invalid_argument("Invalid fractional notation");
    }

    double basePrice = std::stod(fractional.substr(0, dashPos));
    int xy = std::stoi(fractional.substr(dashPos + 1, 2));
    char zChar = fractional[dashPos + 3];
    int z = (zChar == '+') ? 4 : zChar - '0';

    if (xy < 0 || xy > 31 || z < 0 || z > 7) {
        throw std::invalid_argument("Invalid fractional components");
    }

    return basePrice + (xy / 32.0) + (z / 256.0);
}

/**
 * Converts a decimal price into a fractional representation ("base-XYz").
 *   - base: integer part
 *   - XY: fraction in 32nds
 *   - z: fraction in 256ths (0..7), with 4 replaced by '+'
 */
std::string price2string(double decimal) {
    int basePrice = static_cast<int>(decimal);
    double fractionalPart = decimal - basePrice;
    int xy = static_cast<int>(fractionalPart * 32);
    int z = static_cast<int>((fractionalPart * 256)) % 8;
    char zChar = (z == 4) ? '+' : '0' + z;
    return std::to_string(basePrice) + "-" + (xy < 10 ? "0" : "") +
           std::to_string(xy) + zChar;
}

/**
 * Retrieves a Bond object given an integer maturity.
 * Looks up the (CUSIP, date) from bondMap and the coupon rate from bondCoupon,
 * then constructs a Bond. Ticker is "US<Maturity>Y" (e.g., "US2Y").
 */
Bond GetBond(int maturity) {
    string id = bondMap.at(maturity).first;
    string ticker = "US" + to_string(maturity) + "Y";
    return Bond(id, CUSIP, ticker, bondCoupon.at(id), bondMap.at(maturity).second);
}

/**
 * Retrieves a Bond object given its CUSIP string.
 * Uses bondId to find the integer maturity, then calls GetBond(int).
 */
Bond GetBond(string _id) {
    int _mat = bondId.at(_id);
    return GetBond(_mat);
}

#endif /* UTILITY_HPP */
