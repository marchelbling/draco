#include <cstdint>// uint32_t
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

#include "core/symbol_encoding.h"
#include "core/symbol_decoding.h"
#include "core/encoder_buffer.h"
#include "core/decoder_buffer.h"

using namespace draco;

std::vector<uint32_t> read_binary(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    std::vector<uint32_t> values;
    for(unsigned int i = 0 ; i < 9 ; ++ i) {
        values.push_back(0);
        file.read(reinterpret_cast<char*>(&values.back()), sizeof(uint32_t));
    }
    return values;
}

void write_encoded_binary(const std::string& path, const char* content, int size, int num_values, int num_components) {
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(&num_values), sizeof(int));
    file.write(reinterpret_cast<const char*>(&num_components), sizeof(int));
    file.write(reinterpret_cast<const char*>(&size), sizeof(int));
    file.write(reinterpret_cast<const char*>(content), size * sizeof(char));
}

void encode(const std::string& input, const std::string& output, int num_components) {
    std::vector<uint32_t> raw = read_binary(input);
    int num_values = raw.size() / num_components;

    EncoderBuffer buffer;
    EncodeSymbols(&raw[0], num_values, num_components, &buffer);
    write_encoded_binary(output, buffer.data(), buffer.size(), num_values, num_components);
}


std::vector<char> read_encoded_binary(const std::string& path, int& num_values, int& num_components) {
    std::ifstream file(path, std::ios::binary);
    int size(0);

    file.read(reinterpret_cast<char*>(&num_values), sizeof(int));
    file.read(reinterpret_cast<char*>(&num_components), sizeof(int));
    file.read(reinterpret_cast<char*>(&size), sizeof(int));
    std::vector<char> values;
    for(int i = 0 ; i < size ; ++ i) {
        values.push_back(0);
        file.read(reinterpret_cast<char*>(&values.back()), sizeof(char));
    }
    return values;
}

void write_binary(const std::string& path, const uint32_t* content, int num_values, int num_components) {
    std::ofstream file(path, std::ios::binary);
    file.write((char*)content, num_values * num_components * sizeof(uint32_t));
}

void decode(const std::string& input, const std::string& output) {
    int num_values(0), num_components(0);
    std::vector<char> compressed = read_encoded_binary(input, num_values, num_components);
    std::vector<uint32_t> decompressed(num_values * num_components);

    DecoderBuffer buffer;
    buffer.Init(&compressed[0], compressed.size());

    if(!DecodeSymbols(num_values, num_components, &buffer, &decompressed[0])) {
        std::cout << "Decoding failed." << std::endl;
    }
    else {
        std::cout << "Decoding succeeded: showing first values...\n";
        for(unsigned int i = 0 ; i < std::min(3, num_values * num_components) ; ++ i) {
            std::cout << "value n. " << i << ": " << decompressed[i] << std::endl;
        }
    }
    write_binary(output, &decompressed[0], num_values, num_components);
}


int main(int argc, char** argv) {
    std::cout << "Draco cli..." << std::endl;
    std::string  input(argv[1]);
    int num_components = 3;
    if(argc > 2) {
        num_components = atoi(argv[2]);
    }

    // write dummy file
    {
        std::ofstream out(input, std::ios::binary);
        uint32_t a(1), b(2), c(3);
        out.write(reinterpret_cast<char*>(&a), sizeof(uint32_t));
        out.write(reinterpret_cast<char*>(&a), sizeof(uint32_t));
        out.write(reinterpret_cast<char*>(&a), sizeof(uint32_t));

        out.write(reinterpret_cast<char*>(&a), sizeof(uint32_t));
        out.write(reinterpret_cast<char*>(&b), sizeof(uint32_t));
        out.write(reinterpret_cast<char*>(&c), sizeof(uint32_t));

        out.write(reinterpret_cast<char*>(&b), sizeof(uint32_t));
        out.write(reinterpret_cast<char*>(&a), sizeof(uint32_t));
        out.write(reinterpret_cast<char*>(&a), sizeof(uint32_t));
    }

    std::cout << ">>> input: " << input << std::endl;
    std::cout << ">>> num_components: " << num_components << std::endl;

    std::string compressed(input + ".enc");
    std::string decompressed(compressed + ".dec");

    encode(input, compressed, num_components);
    decode(compressed, decompressed);
    return 0;
}
