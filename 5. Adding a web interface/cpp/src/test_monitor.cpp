#include "../include/temperature_measurement.h"
#include "../include/rotating_log.h"
#include "../include/data_processor.h"
#include "../include/temperature_validator.h"
#include <iostream>
#include <vector>
#include <random>
#include <thread>
#include <chrono>

void test_basic_functionality() {
    std::cout << "=== Testing Basic Functionality ===" << std::endl;
    
    // 1. Test TemperatureMeasurement
    TemperatureMeasurement tm1(22.5);
    std::cout << "Measurement: " << tm1.to_string() << std::endl;
    
    // 2. Test DataProcessor
    DataProcessor processor(5);
    for (int i = 0; i < 7; i++) {
        processor.add_measurement(TemperatureMeasurement(20.0 + i));
        processor.add_to_hourly(20.0 + i);
        processor.add_to_daily(20.0 + i);
        std::cout << "Added measurement: " << (20.0 + i) << " C" << std::endl;
        if (processor.should_flush()) {
            std::cout << "  -> Buffer full, should flush" << std::endl;
            auto data = processor.flush();
            std::cout << "  -> Flushed " << data.size() << " measurements" << std::endl;
        }
    }
    
    // 3. Test averages
    std::cout << "Hourly average: " << processor.get_hourly_average() << " C" << std::endl;
    std::cout << "Daily average: " << processor.get_daily_average() << " C" << std::endl;
    
    // 4. Test RotatingLog
    RotatingLog log("test.log", RotatingLog::RetentionPolicy::LAST_24_HOURS);
    log.write("Test log entry 1");
    log.write(TemperatureMeasurement(25.5));
    
    // 5. Test TemperatureValidator
    TemperatureValidator validator;
    auto result1 = validator.validate("22.5 ABC123");
    std::cout << "Validation result: " << (result1.valid ? "VALID" : "INVALID") 
              << " Temp: " << result1.temperature << std::endl;
    
    auto result2 = validator.validate("invalid data");
    std::cout << "Validation result: " << (result2.valid ? "VALID" : "INVALID") 
              << " Error: " << result2.error_message << std::endl;
    
    std::cout << "=== Test Completed ===" << std::endl;
}

void test_simulation() {
    std::cout << "\n=== Running Simulation ===" << std::endl;
    
    DataProcessor processor(10);
    TemperatureValidator validator;
    RotatingLog measurements_log("simulation_measurements.log", 
                                RotatingLog::RetentionPolicy::LAST_24_HOURS);
    RotatingLog hourly_log("simulation_hourly.log", 
                          RotatingLog::RetentionPolicy::LAST_30_DAYS);
    RotatingLog daily_log("simulation_daily.log", 
                         RotatingLog::RetentionPolicy::CURRENT_YEAR);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> dist(22.0, 3.0);
    
    int measurement_count = 0;
    
    for (int i = 0; i < 30; i++) {  // Simulate 30 "minutes"
        // Generate temperature
        double temp = dist(gen);
        if (temp < -40.0) temp = -40.0;
        if (temp > 60.0) temp = 60.0;
        
        // Create simulated data packet
        std::string data = std::to_string(temp).substr(0, 5) + " CHECKSUM";
        
        // Validate
        auto result = validator.validate(data);
        if (result.valid) {
            measurement_count++;
            
            TemperatureMeasurement measurement(result.temperature);
            processor.add_measurement(measurement);
            processor.add_to_hourly(result.temperature);
            processor.add_to_daily(result.temperature);
            measurements_log.write(measurement);
            
            std::cout << "[" << i << " min] Temperature: " << result.temperature << " C" << std::endl;
            
            // Every 6 "minutes" simulate an hour
            if (i % 6 == 5) {
                double avg = processor.get_hourly_average();
                std::cout << "  -> Hourly average: " << avg << " C" << std::endl;
                hourly_log.write(TemperatureMeasurement(avg));
                processor.reset_hourly();
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Final daily average
    double daily_avg = processor.get_daily_average();
    std::cout << "\nDaily average: " << daily_avg << " C" << std::endl;
    daily_log.write(TemperatureMeasurement(daily_avg));
    
    std::cout << "Total measurements: " << measurement_count << std::endl;
    std::cout << "=== Simulation Completed ===" << std::endl;
}

int main() {
    std::cout << "Temperature Monitor Test Suite" << std::endl;
    std::cout << "==============================" << std::endl;
    
    test_basic_functionality();
    test_simulation();
    
    std::cout << "\nCheck generated log files:" << std::endl;
    std::cout << "- simulation_measurements.log" << std::endl;
    std::cout << "- simulation_hourly.log" << std::endl;
    std::cout << "- simulation_daily.log" << std::endl;
    
    return 0;
}