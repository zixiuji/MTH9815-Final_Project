//
//  main.cpp
//  TradingSystem
//
//  @author Zixiuji Wang
//

#include "AlgoExecutionService.hpp"
#include "AlgoStreamingService.hpp"
#include "DataGenerator.hpp"
#include "GUIservice.hpp"
#include "executionservice.hpp"
#include "historicaldataservice.hpp"
#include "inquiryservice.hpp"
#include "marketdataservice.hpp"
#include "positionservice.hpp"
#include "pricingservice.hpp"
#include "products.hpp"
#include "riskservice.hpp"
#include "soa.hpp"
#include "streamingservice.hpp"
#include "tradebookingservice.hpp"
#include <iostream>
#include <random>

int main() {
    // Step 1: Generate all data
    std::cout << "====== Data Generating... ======" << std::endl;
    GeneratePrices();
    GenerateTrades();
    GenerateInquiries();
    GenerateMarketData();
    std::cout << "====== Data Generated! ======" << std::endl;

    // Step 2: Use Bond as the productType, register all the service
    std::cout << "====== Services initializing... ======\n";
    MarketDataService<Bond> BondMarketDataService;
    PricingService<Bond> BondPricingService;
    TradeBookingService<Bond> BondTradeBookingService;
    PositionService<Bond> BondPositionService;
    RiskService<Bond> BondRiskService;
    AlgoExecutionService<Bond> BondAlgoExecutionService;
    AlgoStreamingService<Bond> BondAlgoStreamingService;
    ExecutionService<Bond> BondExecutionService;
    StreamingService<Bond> BondStreamingService;
    InquiryService<Bond> BondInquiryService;
    GUIService<Bond> BondGUIService;
    HistoricalDataService<Position<Bond>> BondHistoricalPositionService("Position");
    HistoricalDataService<PV01<Bond>> BondHistoricalRiskService("Risk");
    HistoricalDataService<ExecutionOrder<Bond>> BondHistoricalExecutionService("Execution");
    HistoricalDataService<PriceStream<Bond>> BondHistoricalStreamingService("Streaming");
    HistoricalDataService<Inquiry<Bond>> BondHistoricalInquiryService("Inquiry");
    std::cout << "====== Services initialized! ======\n";

    // Step 3: Link corresponding service
    std::cout << "====== Services linking... ======" << std::endl;
    BondPricingService.AddListener(BondGUIService.GetListener());
    BondPricingService.AddListener(BondAlgoStreamingService.GetListener());
    BondAlgoStreamingService.AddListener(BondStreamingService.GetListener());
    BondStreamingService.AddListener(
        BondHistoricalStreamingService.GetServiceListener());
    BondMarketDataService.AddListener(BondAlgoExecutionService.GetListener());
    BondAlgoExecutionService.AddListener(BondExecutionService.GetListener());
    BondExecutionService.AddListener(
        BondHistoricalExecutionService.GetServiceListener());
    BondExecutionService.AddListener(BondTradeBookingService.GetListener());
    BondTradeBookingService.AddListener(BondPositionService.GetListener());
    BondPositionService.AddListener(BondRiskService.GetListener());
    BondPositionService.AddListener(
        BondHistoricalPositionService.GetServiceListener());
    BondRiskService.AddListener(BondHistoricalRiskService.GetServiceListener());
    BondInquiryService.AddListener(
        BondHistoricalInquiryService.GetServiceListener());
    std::cout << "====== Services linked! ======" << std::endl;

    // Step 4: Read data and write to output
    const string dirPath = "Data/Input/";
    ifstream priceData(dirPath + "prices.txt");

    BondPricingService.GetConnector()->Subscribe(priceData);
    ifstream marketData(dirPath + "marketdata.txt");
    BondMarketDataService.GetConnector()->Subscribe(marketData);
    ifstream tradeData(dirPath + "trades.txt");
    BondTradeBookingService.GetConnector()->Subscribe(tradeData);
    ifstream inquiryData(dirPath + "inquiries.txt");
    BondInquiryService.GetConnector()->Subscribe(inquiryData);
    std::cout << "====== All Finished! ======" << std::endl;

    return 0;
}
