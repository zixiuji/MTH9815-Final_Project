//
//  DataGenerator.hpp
//  TradingSystem
//
//  Created by Zixiuji Wang
//

#ifndef DataGenerator_hpp
#define DataGenerator_hpp

#include <boost/date_time/gregorian/gregorian.hpp>
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>
#include "utility.hpp" // Assumes this header provides bondMap, price2string, etc.

using namespace std;
using namespace boost::gregorian;

// Set the scale of the generated data
int DATASIZE = 10000;

// Specify the output directory
const string dirPath = "Data/Input/";

/**
 * GeneratePrices()
 *
 * This function creates "prices.txt" in overwrite mode.
 * For each bond in bondMap, it generates a series of bid/ask prices
 * oscillating around [99.0, 101.0], with a minimum tick of 1/256.
 */
void GeneratePrices() {
    const string filePath = dirPath + "prices.txt";
    // Open the file in overwrite (trunc) mode
    ofstream file(filePath, ios::out | ios::trunc);
    if (!file.is_open()) {
        cerr << "Error: Unable to open or create file at " << filePath << endl;
        return;
    }

    const int orderSize = DATASIZE;
    const double minTick = 1.0 / 256.0;
    const double LOW_LIMIT = 99.0 + minTick * 2.0;
    const double UPPER_LIMIT = 101.0 - minTick * 2.0;

    for (const auto& [mat, bond] : bondMap) {
        double central_price = LOW_LIMIT;
        bool up = true;
        thread_local random_device rd;
        thread_local mt19937_64 gen(rd());
        thread_local bernoulli_distribution d(0.5);

        for (int i = 0; i < orderSize; ++i) {
            double ask = central_price + minTick;
            double bid = central_price - minTick;

            // Randomly adjust ask or bid by an additional tick
            if (d(gen)) ask += minTick;
            if (d(gen)) bid -= minTick;

            // Oscillate the central price
            central_price += (up ? minTick : -minTick);
            if (central_price >= UPPER_LIMIT) up = false;
            if (central_price <= LOW_LIMIT) up = true;

            // Write (BondID, Bid, Ask) to file
            file << bond.first << ","
                 << price2string(bid) << ","
                 << price2string(ask) << endl;
        }
    }

    file.close();
    cout << "prices.txt Generated (overwritten)!\n";
}

/**
 * GenerateMarketData()
 *
 * This function creates "marketdata.txt" in overwrite mode.
 * For each bond in bondMap, it simulates a 5-level order book around
 * the price range [99.0, 101.0].
 */
void GenerateMarketData() {
    const string filePath = dirPath + "marketdata.txt";
    ofstream file(filePath, ios::out | ios::trunc);
    if (!file.is_open()) {
        cerr << "Error: Unable to open or create file at " << filePath << endl;
        return;
    }

    const int orderSize = DATASIZE / 10;
    const double minTick = 1.0 / 256.0;
    const double basePrice = 99.0;

    for (const auto& [mat, bond] : bondMap) {
        double price = basePrice;
        bool increasing = true;

        for (int i = 0; i < orderSize; ++i) {
            // Simulate 1 ~ 5 levels for bid/offer
            for (int level = 1; level <= 5; ++level) {
                double spread = level * minTick;
                double bidPrice = price - spread;
                double askPrice = price + spread;
                int size = level * 10000000;  // e.g., 10 million, 20 million, etc.

                file << bond.first << ","
                     << price2string(bidPrice) << ","
                     << size << ",BID" << endl;

                file << bond.first << ","
                     << price2string(askPrice) << ","
                     << size << ",OFFER" << endl;
            }

            // Price oscillation
            if (price >= 101.0 - minTick) increasing = false;
            if (price <= 99.0 + minTick) increasing = true;
            price += (increasing ? minTick : -minTick);
        }
    }

    file.close();
    cout << "marketdata.txt Generated (overwritten)!\n";
}

/**
 * GenerateInquiries()
 *
 * This function creates "inquiries.txt" in overwrite mode.
 * For each bond in bondMap, it generates 10 inquiry entries with random
 * prices, buy/sell side, and quantity.
 */
void GenerateInquiries() {
    const string filePath = dirPath + "inquiries.txt";
    ofstream file(filePath, ios::out | ios::trunc);
    if (!file.is_open()) {
        cerr << "Error: Unable to open or create file at " << filePath << endl;
        return;
    }

    thread_local random_device rd;
    thread_local mt19937_64 gen(rd());
    thread_local uniform_real_distribution<double> d(0.0, 1.0);
    const double minTick = 1.0 / 256.0;

    for (const auto& [mat, bond] : bondMap) {
        for (int i = 0; i < 10; ++i) {
            int _n = static_cast<int>(d(gen) * 512);
            double _price = 99.0 + minTick * _n;

            string inquiryId = bond.first + "_INQ" + to_string(i);
            string side = (i % 2 == 0) ? "BUY" : "SELL";
            int quantity = ((i % 5) + 1) * 1000000;

            // Write (InquiryID, BondID, Side, Quantity, Price, Status) to file
            file << inquiryId << ","
                 << bond.first << ","
                 << side << ","
                 << quantity << ","
                 << price2string(_price) << ",RECEIVED"
                 << endl;
        }
    }

    file.close();
    cout << "inquiries.txt Generated (overwritten)!\n";
}

/**
 * GenerateTrades()
 *
 * This function creates "trades.txt" in overwrite mode.
 * For each bond in bondMap, it generates 10 trade entries with random
 * trade IDs, buy/sell side, quantity, and a random "book" name.
 */
void GenerateTrades() {
    const string filePath = dirPath + "trades.txt";
    ofstream file(filePath, ios::out | ios::trunc);
    if (!file.is_open()) {
        cerr << "Error: Unable to open or create file at " << filePath << endl;
        return;
    }

    thread_local random_device rd;
    thread_local mt19937_64 gen(rd());
    thread_local uniform_real_distribution<double> d(0.0, 1.0);
    const double minTick = 1.0 / 256.0;

    for (const auto& [mat, bond] : bondMap) {
        for (int i = 0; i < 10; ++i) {
            string tradeId = bond.first + "_TRADE" + to_string(i);
            string side = (i % 2 == 0) ? "BUY" : "SELL";
            int quantity = ((i % 5) + 1) * 1000000;

            // Randomly choose a trade book index from [1..3]
            int _market = (static_cast<int>(d(gen) * 3) % 3) + 1;
            string _book_name = "TRSY" + to_string(_market);

            // Random price based on minTick offset
            int _n = static_cast<int>(d(gen) * 512);
            double _price = 99.0 + minTick * _n;

            // Write (BondID, TradeID, Price, BookName, Quantity, Side) to file
            file << bond.first << ","
                 << tradeId << ","
                 << price2string(_price) << ","
                 << _book_name << ","
                 << quantity << ","
                 << side
                 << endl;
        }
    }

    file.close();
    cout << "trades.txt Generated (overwritten)!\n";
}

#endif /* DataGenerator_hpp */
