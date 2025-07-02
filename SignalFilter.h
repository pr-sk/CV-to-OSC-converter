#pragma once

#include <vector>
#include <memory>
#include <cmath>
#include <algorithm>
#include <deque>

enum class FilterType {
    None,
    LowPass,
    HighPass,
    BandPass,
    Notch,
    MovingAverage,
    Median,
    Exponential
};

class IFilter {
public:
    virtual ~IFilter() = default;
    virtual float process(float input) = 0;
    virtual void reset() = 0;
    virtual FilterType getType() const = 0;
    virtual std::string getName() const = 0;
};

// Low-pass Butterworth filter
class LowPassFilter : public IFilter {
private:
    float cutoffFreq;
    float sampleRate;
    float alpha;
    float prevOutput;
    bool initialized;

public:
    LowPassFilter(float cutoff = 10.0f, float sampleRate = 44100.0f) 
        : cutoffFreq(cutoff), sampleRate(sampleRate), prevOutput(0.0f), initialized(false) {
        calculateCoefficients();
    }
    
    void setCutoffFrequency(float cutoff) {
        cutoffFreq = cutoff;
        calculateCoefficients();
    }
    
    float process(float input) override {
        if (!initialized) {
            prevOutput = input;
            initialized = true;
            return input;
        }
        
        prevOutput = alpha * input + (1.0f - alpha) * prevOutput;
        return prevOutput;
    }
    
    void reset() override {
        prevOutput = 0.0f;
        initialized = false;
    }
    
    FilterType getType() const override { return FilterType::LowPass; }
    std::string getName() const override { 
        return "LowPass(" + std::to_string(cutoffFreq) + "Hz)"; 
    }

private:
    void calculateCoefficients() {
        float rc = 1.0f / (2.0f * M_PI * cutoffFreq);
        float dt = 1.0f / sampleRate;
        alpha = dt / (rc + dt);
    }
};

// High-pass Butterworth filter
class HighPassFilter : public IFilter {
private:
    float cutoffFreq;
    float sampleRate;
    float alpha;
    float prevInput;
    float prevOutput;
    bool initialized;

public:
    HighPassFilter(float cutoff = 1.0f, float sampleRate = 44100.0f) 
        : cutoffFreq(cutoff), sampleRate(sampleRate), 
          prevInput(0.0f), prevOutput(0.0f), initialized(false) {
        calculateCoefficients();
    }
    
    void setCutoffFrequency(float cutoff) {
        cutoffFreq = cutoff;
        calculateCoefficients();
    }
    
    float process(float input) override {
        if (!initialized) {
            prevInput = input;
            prevOutput = 0.0f;
            initialized = true;
            return 0.0f;
        }
        
        prevOutput = alpha * (prevOutput + input - prevInput);
        prevInput = input;
        return prevOutput;
    }
    
    void reset() override {
        prevInput = 0.0f;
        prevOutput = 0.0f;
        initialized = false;
    }
    
    FilterType getType() const override { return FilterType::HighPass; }
    std::string getName() const override { 
        return "HighPass(" + std::to_string(cutoffFreq) + "Hz)"; 
    }

private:
    void calculateCoefficients() {
        float rc = 1.0f / (2.0f * M_PI * cutoffFreq);
        float dt = 1.0f / sampleRate;
        alpha = rc / (rc + dt);
    }
};

// Moving average filter
class MovingAverageFilter : public IFilter {
private:
    std::deque<float> buffer;
    size_t windowSize;
    float sum;

public:
    MovingAverageFilter(size_t window = 32) : windowSize(window), sum(0.0f) {
        buffer.reserve(windowSize);
    }
    
    void setWindowSize(size_t window) {
        windowSize = window;
        reset();
    }
    
    float process(float input) override {
        buffer.push_back(input);
        sum += input;
        
        if (buffer.size() > windowSize) {
            sum -= buffer.front();
            buffer.pop_front();
        }
        
        return sum / buffer.size();
    }
    
    void reset() override {
        buffer.clear();
        sum = 0.0f;
    }
    
    FilterType getType() const override { return FilterType::MovingAverage; }
    std::string getName() const override { 
        return "MovingAverage(" + std::to_string(windowSize) + ")"; 
    }
};

// Median filter for noise removal
class MedianFilter : public IFilter {
private:
    std::deque<float> buffer;
    size_t windowSize;

public:
    MedianFilter(size_t window = 5) : windowSize(window) {
        buffer.reserve(windowSize);
    }
    
    void setWindowSize(size_t window) {
        windowSize = window;
        reset();
    }
    
    float process(float input) override {
        buffer.push_back(input);
        
        if (buffer.size() > windowSize) {
            buffer.pop_front();
        }
        
        // Calculate median
        std::vector<float> sorted(buffer.begin(), buffer.end());
        std::sort(sorted.begin(), sorted.end());
        
        size_t size = sorted.size();
        if (size % 2 == 0) {
            return (sorted[size/2 - 1] + sorted[size/2]) / 2.0f;
        } else {
            return sorted[size/2];
        }
    }
    
    void reset() override {
        buffer.clear();
    }
    
    FilterType getType() const override { return FilterType::Median; }
    std::string getName() const override { 
        return "Median(" + std::to_string(windowSize) + ")"; 
    }
};

// Exponential moving average filter
class ExponentialFilter : public IFilter {
private:
    float alpha;
    float prevOutput;
    bool initialized;

public:
    ExponentialFilter(float smoothing = 0.1f) : alpha(smoothing), prevOutput(0.0f), initialized(false) {
        alpha = std::max(0.001f, std::min(1.0f, alpha)); // Clamp to valid range
    }
    
    void setSmoothing(float smoothing) {
        alpha = std::max(0.001f, std::min(1.0f, smoothing));
    }
    
    float process(float input) override {
        if (!initialized) {
            prevOutput = input;
            initialized = true;
            return input;
        }
        
        prevOutput = alpha * input + (1.0f - alpha) * prevOutput;
        return prevOutput;
    }
    
    void reset() override {
        prevOutput = 0.0f;
        initialized = false;
    }
    
    FilterType getType() const override { return FilterType::Exponential; }
    std::string getName() const override { 
        return "Exponential(" + std::to_string(alpha) + ")"; 
    }
};

// Filter chain for combining multiple filters
class FilterChain : public IFilter {
private:
    std::vector<std::unique_ptr<IFilter>> filters;
    FilterType primaryType;

public:
    FilterChain(FilterType type = FilterType::None) : primaryType(type) {}
    
    void addFilter(std::unique_ptr<IFilter> filter) {
        if (filters.empty()) {
            primaryType = filter->getType();
        }
        filters.push_back(std::move(filter));
    }
    
    void clearFilters() {
        filters.clear();
        primaryType = FilterType::None;
    }
    
    float process(float input) override {
        float output = input;
        for (auto& filter : filters) {
            output = filter->process(output);
        }
        return output;
    }
    
    void reset() override {
        for (auto& filter : filters) {
            filter->reset();
        }
    }
    
    FilterType getType() const override { return primaryType; }
    std::string getName() const override {
        if (filters.empty()) return "EmptyChain";
        
        std::string name = "Chain[";
        for (size_t i = 0; i < filters.size(); ++i) {
            if (i > 0) name += "->";
            name += filters[i]->getName();
        }
        name += "]";
        return name;
    }
    
    size_t getFilterCount() const { return filters.size(); }
};

// Factory for creating common filter configurations
class FilterFactory {
public:
    // Create a filter optimized for CV signals (typically low frequency)
    static std::unique_ptr<IFilter> createCVFilter() {
        auto chain = std::make_unique<FilterChain>(FilterType::LowPass);
        chain->addFilter(std::make_unique<MedianFilter>(3));  // Remove spikes
        chain->addFilter(std::make_unique<LowPassFilter>(50.0f, 44100.0f));  // Remove high freq noise
        return std::move(chain);
    }
    
    // Create a filter for audio-rate signals
    static std::unique_ptr<IFilter> createAudioFilter() {
        auto chain = std::make_unique<FilterChain>(FilterType::LowPass);
        chain->addFilter(std::make_unique<HighPassFilter>(20.0f, 44100.0f));  // Remove DC
        chain->addFilter(std::make_unique<LowPassFilter>(20000.0f, 44100.0f));  // Anti-aliasing
        return std::move(chain);
    }
    
    // Create a gentle smoothing filter
    static std::unique_ptr<IFilter> createSmoothingFilter() {
        return std::make_unique<ExponentialFilter>(0.05f);
    }
    
    // Create an aggressive noise reduction filter
    static std::unique_ptr<IFilter> createNoiseReductionFilter() {
        auto chain = std::make_unique<FilterChain>(FilterType::Median);
        chain->addFilter(std::make_unique<MedianFilter>(5));
        chain->addFilter(std::make_unique<MovingAverageFilter>(8));
        chain->addFilter(std::make_unique<LowPassFilter>(100.0f, 44100.0f));
        return std::move(chain);
    }
    
    // Create a filter based on type and parameters
    static std::unique_ptr<IFilter> createFilter(FilterType type, float param1 = 0.0f, float param2 = 0.0f) {
        switch (type) {
            case FilterType::LowPass:
                return std::make_unique<LowPassFilter>(param1 > 0 ? param1 : 10.0f, 
                                                      param2 > 0 ? param2 : 44100.0f);
            case FilterType::HighPass:
                return std::make_unique<HighPassFilter>(param1 > 0 ? param1 : 1.0f, 
                                                       param2 > 0 ? param2 : 44100.0f);
            case FilterType::MovingAverage:
                return std::make_unique<MovingAverageFilter>(param1 > 0 ? static_cast<size_t>(param1) : 32);
            case FilterType::Median:
                return std::make_unique<MedianFilter>(param1 > 0 ? static_cast<size_t>(param1) : 5);
            case FilterType::Exponential:
                return std::make_unique<ExponentialFilter>(param1 > 0 ? param1 : 0.1f);
            default:
                return nullptr;
        }
    }
};
