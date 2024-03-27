#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <iomanip>

using namespace std;
using namespace std::chrono;

// A Tree node
struct Node
{
    char ch;
    int freq;
    Node* left, * right;
};

// Function to allocate a new tree node
Node* createNode(char ch, int freq, Node* left, Node* right)
{
    Node* node = new Node();
    node->ch = ch;
    node->freq = freq;
    node->left = left;
    node->right = right;
    return node;
}

// Comparison object to be used to order the heap
struct CompareNodes
{
    bool operator()(Node* l, Node* r)
    {
        // highest priority item has lowest frequency
        return l->freq > r->freq;
    }
};

// Builds Huffman Tree
Node* buildHuffmanTree(const string& text)
{
    // Count frequency of appearance of each character
    unordered_map<char, int> frequency;
    for (char ch : text) {
        frequency[ch]++;
    }

    // Create a priority queue to store live nodes of the Huffman tree
    priority_queue<Node*, vector<Node*>, CompareNodes> pq;

    // Create a leaf node for each character and add it to the priority queue
    for (auto pair : frequency) {
        pq.push(createNode(pair.first, pair.second, nullptr, nullptr));
    }

    // Construct the Huffman tree by combining nodes
    while (pq.size() > 1) {
        Node* left = pq.top();
        pq.pop();
        Node* right = pq.top();
        pq.pop();

        // Create a new internal node with these two nodes as children
        // and with frequency equal to the sum of the two nodes' frequencies
        int sum = left->freq + right->freq;
        pq.push(createNode('\0', sum, left, right));
    }

    // Return the root of the Huffman tree
    return pq.top();
}

// Traverse the Huffman Tree and store Huffman Codes in a map
void generateHuffmanCodes(Node* root, string str, unordered_map<char, string>& huffmanCodes)
{
    if (root == nullptr) {
        return;
    }

    // Found a leaf node
    if (!root->left && !root->right) {
        huffmanCodes[root->ch] = str;
        return;
    }

    // Recursively traverse the tree and append '0' or '1' to the current code
    generateHuffmanCodes(root->left, str + "0", huffmanCodes);
    generateHuffmanCodes(root->right, str + "1", huffmanCodes);
}

// Compress the text using Huffman Codes and save to output file
void compressText(const string& text, const unordered_map<char, string>& huffmanCodes, const string& outputFilename)
{
    ofstream outputFile(outputFilename, ios::binary);
    if (!outputFile.is_open()) {
        cout << "Unable to open output file\n";
        return;
    }

    // Write the number of distinct characters in the original text
    size_t numDistinctChars = huffmanCodes.size();
    outputFile.write(reinterpret_cast<const char*>(&numDistinctChars), sizeof(numDistinctChars));

    // Write the Huffman Codes table to the output file
    for (const auto& pair : huffmanCodes) {
        char ch = pair.first;
        const string& code = pair.second;
        size_t codeLength = code.size();
        outputFile.write(reinterpret_cast<const char*>(&ch), sizeof(ch));
        outputFile.write(reinterpret_cast<const char*>(&codeLength), sizeof(codeLength));
        outputFile.write(code.c_str(), codeLength);
    }

    // Compress the text
    vector<char> compressedData; // Accumulate compressed bytes
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

    // Flush any remaining bits in the buffer
    if (bufferIndex > 0) {
        compressedData.push_back(buffer);
    }

    // Write the compressed bytes to the output file
    outputFile.write(compressedData.data(), compressedData.size());

    outputFile.close();
}

int main(int argc, char* argv[])
{
    if (argc != 3) {
        cout << "Usage: " << argv[0] << " <inputFileName> <numIterations>\n";
        return 1;
    }

    string inputFilename = argv[1];
    int numIterations = atoi(argv[2]);

    // Read the input text file
    ifstream inputFile(inputFilename);
    if (!inputFile.is_open()) {
        cout << "Unable to open input file\n";
        return 1;
    }

    string text((istreambuf_iterator<char>(inputFile)), (istreambuf_iterator<char>()));
    inputFile.close();

    for (int i = 0; i < numIterations; ++i) {
        auto start = high_resolution_clock::now(); // Start measuring time

        // Build the Huffman tree
        Node* root = buildHuffmanTree(text);

        // Traverse the Huffman tree and store Huffman Codes in a map
        unordered_map<char, string> huffmanCodes;
        generateHuffmanCodes(root, "", huffmanCodes);

        // Compress the text and save to output file
        string outputFilename = "compressed_" + to_string(i + 1) + ".bin";
        compressText(text, huffmanCodes, outputFilename);

        auto stop = high_resolution_clock::now(); // Stop measuring time
        auto duration = duration_cast<milliseconds>(stop - start);
        cout << "Execution Time (Iteration " << i + 1 << "): " << fixed << setprecision(10)
             << duration.count() / 1000.0 << " seconds\n";
    }

    return 0;
}
