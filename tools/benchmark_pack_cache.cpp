#include <nlohmann/json.hpp>

#include <Windows.h>
#include <Psapi.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using Clock = std::chrono::steady_clock;

static std::uint64_t hash_file(const std::filesystem::path& path)
{
	std::ifstream input(path, std::ios::binary);
	std::uint64_t hash = 14695981039346656037ull;
	char buffer[64 * 1024];
	while (input) {
		input.read(buffer, sizeof(buffer));
		for (std::streamsize i = 0; i < input.gcount(); ++i) {
			hash ^= static_cast<unsigned char>(buffer[i]);
			hash *= 1099511628211ull;
		}
	}
	return hash;
}

static double elapsed(Clock::time_point start)
{
	return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

static std::size_t working_set()
{
	PROCESS_MEMORY_COUNTERS counters{};
	return GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)) ? counters.WorkingSetSize : 0;
}

int main()
{
	std::cout << std::unitbuf;
	const std::vector<std::size_t> scales{ 1000, 5000, 20000 };
	std::cout << "definitions,cold_ms,warm_ms,invalidation_ms,cache_bytes,working_set_bytes\n";
	for (const auto count : scales) {
		auto root = std::filesystem::temp_directory_path() / ("naf-pack-cache-benchmark-" + std::to_string(GetCurrentProcessId()) + "-" + std::to_string(count));
		std::filesystem::create_directories(root);
		std::vector<std::filesystem::path> files;
		files.reserve(count);
		for (std::size_t i = 0; i < count; ++i) {
			auto path = root / (std::to_string(i) + ".naf.json");
			std::ofstream output(path);
			output << R"({"format":"NAFExtendedAnimation","schemaVersion":1,"animation":{"id":"bench.)" << i
				<< R"(","actors":[{"requirements":{"skeleton":"Human"},"expectedBodyType":"Any","file":"Example\\clip.hkx"}]},"position":{"id":"bench.)" << i
				<< R"(","displayName":"Benchmark","tags":["benchmark"]}})";
			files.push_back(std::move(path));
		}

		auto start = Clock::now();
		nlohmann::json definitions = nlohmann::json::array();
		for (const auto& path : files) {
			std::ifstream input(path);
			nlohmann::json source;
			input >> source;
			definitions.push_back({ { "source", path.filename().generic_string() }, { "animationID", source["animation"]["id"] },
				{ "positionID", source["position"]["id"] }, { "actors", source["animation"]["actors"] }, { "displayName", source["position"]["displayName"] } });
		}
		auto payload = nlohmann::json::to_cbor({ { "cacheVersion", 1 }, { "schemaVersion", 1 }, { "definitions", definitions } });
		const auto cold = elapsed(start);
		const auto cache = root / "NAFPackCache.bin";
		{ std::ofstream output(cache, std::ios::binary); output.write(reinterpret_cast<const char*>(payload.data()), payload.size()); }

		start = Clock::now();
		std::uint64_t manifestHash{};
		for (const auto& path : files) manifestHash ^= hash_file(path);
		std::ifstream input(cache, std::ios::binary);
		std::vector<std::uint8_t> bytes(std::filesystem::file_size(cache));
		input.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
		auto restored = nlohmann::json::from_cbor(bytes);
		const auto warm = elapsed(start);
		input.close();

		{ std::fstream changed(files.front(), std::ios::in | std::ios::out | std::ios::binary); changed.seekp(20); const char oldByte = 'Z'; changed.write(&oldByte, 1); }
		start = Clock::now();
		std::uint64_t changedHash{};
		for (const auto& path : files) changedHash ^= hash_file(path);
		if (changedHash == manifestHash) return 2;
		for (const auto& path : files) { std::ifstream source(path); nlohmann::json parsed; try { source >> parsed; } catch (...) {} }
		const auto invalidation = elapsed(start);

		std::cout << count << ',' << cold << ',' << warm << ',' << invalidation << ',' << payload.size() << ',' << working_set() << '\n';
		std::filesystem::remove_all(root);
	}
}
