#pragma once

#include "string_processing.h"
#include "document.h"
#include "log_duration.h"
#include "concurrent_map.h"

#include <algorithm>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <cmath>
#include <utility>
#include <numeric>
#include <execution>
#include <string_view>
#include <functional>

using namespace std::literals::string_literals;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

const double MAX_DIFF = 1e-6;

enum class DocumentStatus
{
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer
{

public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);

    explicit SearchServer(std::string stop_words_text);

    explicit SearchServer(std::string_view stop_words_text);

    void AddDocument(int document_id, std::string_view document, DocumentStatus status,
        const std::vector<int>& ratings);

    template<class ExecutionPolicy>
    void RemoveDocument(ExecutionPolicy&& policy, int document_id);

    void RemoveDocument(int document_id);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query,
        DocumentPredicate document_predicate) const;

    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;

    template <typename DocumentPredicate, typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query,
        DocumentPredicate document_predicate) const;

    template<typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query, DocumentStatus status) const;

    template<typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query) const;


    int GetDocumentCount() const;

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    using SetIterator = std::set<int>::const_iterator;

    SetIterator begin() const;

    SetIterator end() const;


    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query,
        int document_id) const;

    template<class ExecutionPolicy>
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(ExecutionPolicy&& policy, std::string_view raw_query,
        int document_id) const;

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    std::set<std::string, std::less<>> word_collector_;
    const std::set<std::string, std::less<>> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, std::map<std::string_view, double>> document_to_word_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;

    bool IsStopWord(std::string_view word) const;

    static bool IsValidWord(std::string_view word);

    std::vector<std::string_view> SplitIntoWordsNoStopView(std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string_view text) const;

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    Query ParseQuery(std::string_view text, bool parallel_policy = 0) const;



    void Deduplicator(std::vector<std::string_view>& vec) const;

    double ComputeWordInverseDocumentFreq(std::string_view word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query,
        DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate, typename ExecutionPolicy>
    std::vector<Document> FindAllDocuments(ExecutionPolicy&& policy, const Query& query,
        DocumentPredicate document_predicate) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
{
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument("Some of stop words are invalid"s);
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query,
    DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(raw_query);

    auto matched_documents = FindAllDocuments(query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < MAX_DIFF) {
                return lhs.rating > rhs.rating;
            }
            else {
                return lhs.relevance > rhs.relevance;
            }
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template <typename DocumentPredicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query,
    DocumentPredicate document_predicate) const
{
    if(typeid(policy) == typeid(std::execution::seq))
    {
        return SearchServer::FindTopDocuments(raw_query, document_predicate);
    }

    const auto query = ParseQuery(raw_query);

    auto matched_documents = FindAllDocuments(policy, query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < MAX_DIFF) {
                return lhs.rating > rhs.rating;
            }
            else {
                return lhs.relevance > rhs.relevance;
            }
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template<typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query, DocumentStatus status) const
{
    if (typeid(policy) == typeid(std::execution::seq))
    {
        return SearchServer::FindTopDocuments(raw_query, status);
    }

    return SearchServer::FindTopDocuments(
        policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

template<typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query) const
{
    if (typeid(policy) == typeid(std::execution::seq))
    {
        return SearchServer::FindTopDocuments(raw_query);
    }

    return SearchServer::FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query,
    DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}

template <typename DocumentPredicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindAllDocuments(ExecutionPolicy&& policy, const Query& query,
    DocumentPredicate document_predicate) const
{
    if(typeid(policy) == typeid(std::execution::seq))
    {
        return FindAllDocuments(query, document_predicate);
    }

    ConcurrentMap<int, double> document_to_relevance;
    for_each(policy, query.plus_words.begin(), query.plus_words.end(), [&](std::string_view word)
        {
            if (word_to_document_freqs_.count(word))
            {
                const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                for_each(word_to_document_freqs_.at(word).begin(), word_to_document_freqs_.at(word).end(), [&](const std::pair<int, double> id_to_freq)
                    {
                        const auto& document_data = documents_.at(id_to_freq.first);
                        if (document_predicate(id_to_freq.first, document_data.status, document_data.rating)) {
                            document_to_relevance[id_to_freq.first] += id_to_freq.second * inverse_document_freq;
                        }
                    });
            }
        });

    for_each(policy, query.minus_words.begin(), query.minus_words.end(), [&](std::string_view word)
        {
            if (word_to_document_freqs_.count(word))
            {
                for_each(word_to_document_freqs_.at(word).begin(), word_to_document_freqs_.at(word).end(), [&](const std::pair<int, double> id_to_freq)
                    {
                        document_to_relevance.erase(id_to_freq.first);
                    });
            }
        });

    std::vector<Document> matched_documents(document_ids_.size());
    std::map<int, double> ordinary_doc_to_rel = document_to_relevance.BuildOrdinaryMap();
    auto new_end = std::transform(policy, ordinary_doc_to_rel.begin(), ordinary_doc_to_rel.end(), matched_documents.begin(), [&](const std::pair<int, double> id_to_relevance)
        {
            return Document{ id_to_relevance.first, id_to_relevance.second, documents_.at(id_to_relevance.first).rating };
        });
    matched_documents.erase(new_end, matched_documents.end());
    return matched_documents;
}

void MatchDocument(const SearchServer& search_server, std::string_view raw_query, int document_id);

void FindTopDocuments(const SearchServer& search_server, std::string_view raw_query);

void AddDocument(SearchServer& search_server, int document_id, std::string_view raw_query, DocumentStatus status,
    const std::vector<int>& ratings);

template<class ExecutionPolicy>
void SearchServer::RemoveDocument(ExecutionPolicy&& policy, int document_id) {
    std::map<std::string_view, double>& id_docs = document_to_word_freqs_.at(document_id);
    std::vector<std::string_view> words_to_remove(id_docs.size());
    std::vector<std::string_view> words_no_freq(id_docs.size());
    std::transform(std::execution::par, id_docs.begin(), id_docs.end(), words_no_freq.begin(),
        [&](auto& word_to_freq)
        {
            return word_to_freq.first;
        });

    
    std::for_each(policy, words_no_freq.begin(), words_no_freq.end(),
        [&](const auto& word)
        {
            word_to_document_freqs_[word].erase(document_id);
        });

    auto erase_pos = std::copy_if(policy, words_no_freq.begin(), words_no_freq.end(), words_to_remove.begin(), [&](std::string_view word)
        {
            return word_to_document_freqs_.at(word).empty();
        });

    words_to_remove.erase(erase_pos, words_to_remove.end());

    document_to_word_freqs_.erase(document_id);

    std::for_each(policy, words_to_remove.begin(), words_to_remove.end(),
        [&](const auto& word)
        {
            word_to_document_freqs_.erase(word);
            word_collector_.erase(std::string(word));
        });

    document_ids_.erase(document_id);
    documents_.erase(document_id); //O(logN)
}

template<class ExecutionPolicy>
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(ExecutionPolicy&& policy, std::string_view raw_query,
    int document_id) const
{
    if (typeid(policy) == typeid(std::execution::seq))
    {
        return MatchDocument(raw_query, document_id);
    }

    Query query = ParseQuery(raw_query, true);

    if (std::any_of(policy, query.minus_words.begin(), query.minus_words.end(),
        [&](std::string_view word)
        {
            return word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id);
        }))
    {
        return { {}, documents_.at(document_id).status };
    }

        std::vector<std::string_view> matched_words(query.plus_words.size());
        auto new_end = std::copy_if(policy, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), [&](std::string_view word)
            {
                return word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id);
            });
        matched_words.resize(std::distance(matched_words.begin(), new_end));
        Deduplicator(matched_words);
        return { matched_words, documents_.at(document_id).status };
}



