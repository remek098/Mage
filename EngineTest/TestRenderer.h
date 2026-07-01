#pragma once
#include "Test.h"

class EngineTest : public Test {
public:
    bool initialize() override;
    void run() override;
    void shutdown() override;
};