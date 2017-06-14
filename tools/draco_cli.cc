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

bool debug(false);


bool file_exists(const std::string& filepath);
void create_dummy_file(std::string& filepath);
void write_encoded_binary(const std::string& filepath, const char* content, int size, int num_values, int num_components);
template<typename T>
unsigned int get_num_values(const std::string& filepath);
std::vector<uint32_t> read_binary(const std::string& filepath, const unsigned int num_values);
void encode(const std::string& input, const std::string& output, int num_components);
void write_encoded_binary(const std::string& filepath, const char* content, int size, int num_values, int num_components);
std::vector<char> read_encoded_binary(const std::string& filepath, int& num_values, int& num_components);
void write_binary(const std::string& filepath, const uint32_t* content, int num_values, int num_components);
void decode(const std::string& input, const std::string& output);



bool file_exists(const std::string& filepath) {
    std::ifstream f(filepath);
    return f.good();
}


void create_dummy_file(std::string& filepath) {
    std::ofstream out(filepath, std::ios::binary);
    uint32_t a(1), b(2), c(3);
    out.write(reinterpret_cast<char*>(&a), sizeof(uint32_t));
    out.write(reinterpret_cast<char*>(&b), sizeof(uint32_t));
    out.write(reinterpret_cast<char*>(&c), sizeof(uint32_t));

    out.write(reinterpret_cast<char*>(&a), sizeof(uint32_t));
    out.write(reinterpret_cast<char*>(&a), sizeof(uint32_t));
    out.write(reinterpret_cast<char*>(&a), sizeof(uint32_t));

    out.write(reinterpret_cast<char*>(&b), sizeof(uint32_t));
    out.write(reinterpret_cast<char*>(&a), sizeof(uint32_t));
    out.write(reinterpret_cast<char*>(&a), sizeof(uint32_t));

    out.write(reinterpret_cast<char*>(&b), sizeof(uint32_t));
    out.write(reinterpret_cast<char*>(&c), sizeof(uint32_t));
    out.write(reinterpret_cast<char*>(&a), sizeof(uint32_t));
}


template<typename T>
unsigned int get_num_values(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::ate | std::ios::binary);
    return file.tellg() / sizeof(T);
}


std::vector<uint32_t> read_binary(const std::string& filepath, const unsigned int num_values) {
    std::ifstream file(filepath, std::ios::binary);
    std::vector<uint32_t> values(num_values, 0);

    for(unsigned int i = 0 ; i < num_values ; ++ i) {
        file.read(reinterpret_cast<char*>(&values[0] + i), sizeof(uint32_t));
        if(debug) { std::cout << "reading values[" << i << "]: " << values[i] << std::endl; }
    }
    return values;
}


void encode(const std::string& input, const std::string& output, int num_components) {
    std::vector<uint32_t> raw = read_binary(input, get_num_values<uint32_t>(input));
    int num_values = raw.size();  //! num_values = #vec * dim(vec)

    draco::EncoderBuffer buffer;
    draco::EncodeSymbols(&raw[0], num_values, num_components, &buffer);
    write_encoded_binary(output, buffer.data(), buffer.size(), num_values, num_components);
}


void write_encoded_binary(const std::string& filepath, const char* content, int size, int num_values, int num_components) {
    std::ofstream file(filepath, std::ios::binary);
    file.write(reinterpret_cast<const char*>(&num_values), sizeof(int));
    file.write(reinterpret_cast<const char*>(&num_components), sizeof(int));
    file.write(reinterpret_cast<const char*>(&size), sizeof(int));
    file.write(reinterpret_cast<const char*>(content), size * sizeof(char));
}


std::vector<char> read_encoded_binary(const std::string& filepath, int& num_values, int& num_components) {
    std::ifstream file(filepath, std::ios::binary);
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


void decode(const std::string& input, const std::string& output) {
    int num_values(0), num_components(0);
    std::vector<char> compressed = read_encoded_binary(input, num_values, num_components);
    std::vector<uint32_t> decompressed(num_values * num_components);

    draco::DecoderBuffer buffer;
    buffer.Init(&compressed[0], compressed.size());

    if(!draco::DecodeSymbols(num_values * num_components, num_components, &buffer, &decompressed[0])) {
        std::cout << "Decoding failed." << std::endl;
    }
    else {
        if(debug) {
            for(unsigned int i = 0 ; i < num_values / num_components ; ++ i) {
                for(unsigned int j = 0 ; j < num_components ; ++ j) {
                    std::cout << "decoding value[" << i << "][" << j << "]: " << decompressed[num_components * i + j] << std::endl;
                }
            }
        }
    }
    write_binary(output, &decompressed[0], num_values, num_components);
}


void write_binary(const std::string& filepath, const uint32_t* content, int num_values, int num_components) {
    std::ofstream file(filepath, std::ios::binary);
    file.write((char*)content, num_values * sizeof(uint32_t));
}


int main(int argc, char** argv) {
    std::string  input(argv[1]);
    int num_components = 1;
    if(argc > 2) {
        num_components = atoi(argv[2]);
    }

    if(!file_exists(input)) { create_dummy_file(input); }

    std::string compressed(input + ".enc" + std::to_string(num_components));
    std::string decompressed(compressed + ".dec" + std::to_string(num_components));

    encode(input, compressed, num_components);
    decode(compressed, decompressed);
    return 0;
}
