#include "search_server.h"
#include "log_duration.h"
#include "process_queries.h"
#include <execution>
#include <iostream>
#include <random>
#include <string>
#include <vector>
using namespace std;

SearchServer ConstructServer()
{
    int n;
    cin >> n;
    vector<string> stop_words;
    for (int i = 0; i < n; ++i)
    {
        string word;
        cin >> word;
        stop_words.push_back(move(word));
    }
    return SearchServer(move(stop_words));
}

void FillDB(SearchServer& s)
{
    s.AddDocument(0, "First document", DocumentStatus::ACTUAL, { 1,3,5 });
    s.AddDocument(1, "Second file in server", DocumentStatus::ACTUAL, {});
    s.AddDocument(2, "Second minus one document", DocumentStatus::ACTUAL, { 1, -2 });
}

int main() {

    SearchServer server = ConstructServer();
    FillDB(server);

    for(const auto& doc : server.FindTopDocuments("document"))
    {
        cout << "Found document: id - " << doc.id << " with average rating - " << doc.rating
            << " and relevance - " << doc.relevance << endl;
    }
    
    cout << "Macthed words for document with id 1:" << endl;
    const auto [words, status] = server.MatchDocument("has file word", 1);
    for(const auto& word : words)
    {
        cout << word << endl;
    }
}