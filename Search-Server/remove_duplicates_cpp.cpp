#include "remove_duplicates_h.h"

using namespace std;

void RemoveDuplicates(SearchServer& search_server) 
{
    set<set<string_view>> documents;
    vector<int> documents_to_remove;

    for (int document_id : search_server) 
    {
        const auto& words = search_server.GetWordFrequencies(document_id);
        set<string_view> doc_words;
        transform(words.begin(), words.end(), inserter(doc_words, doc_words.begin()), [](const auto& el)
        {
            return el.first;
        });

        if (documents.count(doc_words))
        {
            documents_to_remove.push_back(document_id);
        }

        else
        {
            documents.insert(doc_words);
        }
    }
    
    for (int document_id : documents_to_remove) {
        cout << "Found duplicate document id " << document_id << "\n";
        search_server.RemoveDocument(document_id);
    }
}