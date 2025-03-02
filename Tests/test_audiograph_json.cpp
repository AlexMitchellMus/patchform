// test_audiograph_json.cpp

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include "AudioGraph.h"  // Adjust the include path as needed

using json = nlohmann::json;

TEST_CASE("AudioGraph JSON round-trip", "[audiograph][json]") {
    // Create and initialize your AudioGraph instance
    AudioGraph originalGraph;
    originalGraph.setName("Test Graph");
    originalGraph.setParameter("volume", 0.75);
    // ... initialize other properties as needed

    // Serialize the AudioGraph to JSON.
    json j;
    originalGraph.to_json(j);  // Or j = originalGraph; if you overloaded to_json

    // Output the JSON for debugging (optional)
    INFO("Serialized JSON: " << j.dump());

    // Create a new instance and load from the JSON data
    AudioGraph loadedGraph;
    loadedGraph.from_json(j);  // Or use a free function if that fits your design

    // Check that the round-trip preserved data.
    CHECK(loadedGraph.getName() == originalGraph.getName());
    CHECK(loadedGraph.getParameter("volume") == Approx(originalGraph.getParameter("volume")));
    // Add more checks here for each property you expect to be preserved.
}
