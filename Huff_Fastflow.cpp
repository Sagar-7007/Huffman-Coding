#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <unordered_map>
#include <iomanip>
#include <vector>
#include <chrono>
#include <ff/ff.hpp>
#include <ff/parallel_for.hpp>

using namespace std;
using namespace std::chrono;
using namespace ff;

// A Tree node
struct Node {
    char ch;
    int freq;
    Node* left, *right;
};

// Function to allocate a new tree node
Node* createNode(char ch, int freq, Node* left, Node* right) {
    Node* node = new Node();
    node->ch = ch;
    node->freq = freq;
    node->left = left;
    node->right = right;
    return node;
}

// Comparison object to be used to order the heap
struct CompareNodes {
    bool operator()(Node* l, Node* r) {
        return l->freq > r->freq;
    }
};

// Builds Huffman Tree
Node* buildHuffmanTree(const string& text) {
    unordered_map<char, int> frequency;
    for (char ch : text) {
        frequency[ch]++;
    }

    priority_queue<Node*, vector<Node*>, CompareNodes> pq;
    for (auto pair : frequency) {
        pq.push(createNode(pair.first, pair.second, nullptr, nullptr));
    }

    while (pq.size() > 1) {
        Node* left = pq.top(); pq.pop();
        Node* right = pq.top(); pq.pop();
        int sum = left->freq + right->freq;
        pq.push(createNode('\0', sum, left, right));
    }

    return pq.top();
}

// Traverse the Huffman Tree and store Huffman Codes in a map
void generateHuffmanCodes(Node* root, string str, unordered_map<char, string>& huffmanCodes) {
    if (root == nullptr) {
        return;
    }

    if (!root->left && !root->right) {
        huffmanCodes[root->ch] = str;
    } else {
        generateHuffmanCodes(root->left, str + "0", huffmanCodes);
        generateHuffmanCodes(root->right, str + "1", huffmanCodes);
    }
}

// Compress the text using Huffman Codes and save to output file
void compressText(const string& text, const unordered_map<char, string>& huffmanCodes, vector<char>& compressedData) {
    size_t bufferIndex = 0;
    char buffer = 0;
    for (char ch : text) {
        const string& code = huffmanCodes.at(ch);
        for (char bit : code) {
            buffer |= (bit - '0') << (7 - bufferIndex);
            bufferIndex++;
            if (bufferIndex == 8) {
                compressedData.push_back(buffer);
                bufferIndex = 0;
                buffer = 0;
            }
        }
    }

    if (bufferIndex > 0) {
        compressedData.push_back(buffer);
    }
}

void workerFunctionFastFlow(const string& text, int threadId, int numThreads, vector<char>& compressedData) {
    int chunkSize = text.size() / numThreads;
    int startIndex = threadId * chunkSize;
    int endIndex = (threadId == numThreads - 1) ? text.size() : (threadId + 1) * chunkSize;
    string threadText = text.substr(startIndex, endIndex - startIndex);

    Node* root = buildHuffmanTree(threadText);
    unordered_map<char, string> huffmanCodes;
    generateHuffmanCodes(root, "", huffmanCodes);
    compressText(threadText, huffmanCodes, compressedData);
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cout << "Usage: " << argv[0] << " <inputFileName> <numIterations> <numThreads>\n";
        return 1;
    }

    string inputFilename = argv[1];
    int numIterations = atoi(argv[2]);
    int numThreads = atoi(argv[3]);

    ifstream inputFile(inputFilename);
    if (!inputFile.is_open()) {
        cout << "Unable to open input file\n";
        return 1;
    }

    string text((istreambuf_iterator<char>(inputFile)), (istreambuf_iterator<char>()));
    inputFile.close();

    vector<double> iterationTimes(numIterations, 0.0);
    ParallelFor pf(numThreads);

    auto startTotal = high_resolution_clock::now();
    for (int it = 0; it < numIterations; ++it) {
        vector<vector<char>> compressedParts(numThreads);
        auto start = high_resolution_clock::now();

        pf.parallel_for(0, numThreads, 1, [&](const long i) {
            workerFunctionFastFlow(text, i, numThreads, compressedParts[i]);
        }, numThreads);

        auto stop = high_resolution_clock::now();
        iterationTimes[it] = duration_cast<milliseconds>(stop - start).count() / 1000.0;

        // Get current time and format it as YYYYMMDD_HHMMSS
        auto now = system_clock::now();
        auto in_time_t = system_clock::to_time_t(now);
        stringstream ss;
        ss << put_time(localtime(&in_time_t), "%Y%m%d_%H%M%S");
        string timestamp = ss.str();

        // Create a filename that includes the iteration number and timestamp
        string outputFilename = "compressed_" + to_string(it + 1) + "_" + timestamp + ".bin";
        ofstream outputFile(outputFilename, ios::binary);
        if (!outputFile.is_open()) {
            cout << "Unable to open output file: " << outputFilename << endl;
            continue;
        }
        for (const auto& part : compressedParts) {
            outputFile.write(reinterpret_cast<const char*>(part.data()), part.size());
        }
        outputFile.close();
    }

    auto stopTotal = high_resolution_clock::now();
    auto durationTotal = duration_cast<milliseconds>(stopTotal - startTotal).count() / 1000.0;

    cout << "Total Execution Time: " << fixed << setprecision(10) << durationTotal << " seconds\n";
    for (int i = 0; i < numIterations; ++i) {
        cout << "Execution Time (Iteration " << i + 1 << "): " << fixed << setprecision(10) << iterationTimes[i] << " seconds\n";
    }

    return 0;
}