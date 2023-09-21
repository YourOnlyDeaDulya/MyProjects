#pragma once

#include "search_server.h"
#include "document.h"

#include <vector>
#include <deque>

class RequestQueue 
{
public:
    explicit RequestQueue(const SearchServer& search_server);

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(std::string_view raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(std::string_view raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(std::string_view raw_query);

    int GetNoResultRequests() const;

private:
    struct QueryResult {
        std::vector<Document> request_result;
        bool is_empty;
    };

    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& data_base_;
    int empty_count_ = 0;
    int time = 0;
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(std::string_view raw_query, DocumentPredicate document_predicate) {
    ++time;
    std::vector<Document> find_results = data_base_.FindTopDocuments(raw_query, document_predicate);
    bool is_empty = false;
    if (time > min_in_day_)
    {
        if (requests_.front().is_empty == true)
        {
            --empty_count_;
        }
        requests_.pop_front();
    }

    if (find_results.empty())
    {
        ++empty_count_;
        is_empty = true;
    }
    requests_.push_back({ find_results, is_empty });
    return find_results;
}
