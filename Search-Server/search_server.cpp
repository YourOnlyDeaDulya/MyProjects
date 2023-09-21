#include "search_server.h"
#include "string_processing.h"
#include "document.h"

using namespace std;

SearchServer::SearchServer(string stop_words_text)
    : SearchServer(SearchServer(string_view(stop_words_text)))
{
}

SearchServer::SearchServer(string_view str) 
    : SearchServer(
        SplitIntoWordsView(str))
{
}

void SearchServer::AddDocument(int document_id, string_view document, DocumentStatus status,
    const vector<int>& ratings)
{
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id"s);
    }
    const auto words = SplitIntoWordsNoStopView(document);

    const double inv_word_count = 1.0 / words.size();
    for (string_view word : words) {
        auto insert_iter = word_collector_.insert(string(word));
        word_to_document_freqs_[*insert_iter.first][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][*insert_iter.first] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.insert(document_id);
}

void SearchServer::RemoveDocument(int document_id)
{
    RemoveDocument(execution::seq, document_id);
}


vector<Document> SearchServer::FindTopDocuments(string_view raw_query, DocumentStatus status) const
{
    return SearchServer::FindTopDocuments(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

vector<Document> SearchServer::FindTopDocuments(string_view raw_query) const
{
    return SearchServer::FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const
{
    return documents_.size();
}

const map<string_view, double>& SearchServer::GetWordFrequencies(int document_id) const

{
    static const map<string_view, double> empty_map;
    if (document_to_word_freqs_.count(document_id) == 0) {
        return empty_map;
    }
    return document_to_word_freqs_.at(document_id);
}

using SetIterator = std::set<int>::const_iterator;

SetIterator SearchServer::begin() const
{
    return document_ids_.begin();
}

SetIterator SearchServer::end() const
{
    return document_ids_.end();
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(string_view raw_query,
    int document_id) const
{
    const auto query = ParseQuery(raw_query);

    vector<string_view> matched_words;

    for (string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            return { matched_words, documents_.at(document_id).status };
        }
    }
    for (string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }

    return { matched_words, documents_.at(document_id).status };
}

bool SearchServer::IsStopWord(string_view word) const
{
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(string_view word)
{
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

vector<string_view> SearchServer::SplitIntoWordsNoStopView(string_view text) const
{
    vector<string_view> words;
    for (auto& word : SplitIntoWordsView(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Word "s + string(word) + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings)
{
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(string_view text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }
    string_view word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw std::invalid_argument("Query word "s + string(text) + " is invalid");
    }

    return { word, is_minus, IsStopWord(word) };
}

void SearchServer::Deduplicator(std::vector<std::string_view>& vec) const
{
    std::sort(vec.begin(), vec.end());
    auto erase_iterator = std::unique(vec.begin(), vec.end());
    vec.erase(erase_iterator, vec.end());
}

SearchServer::Query SearchServer::ParseQuery(string_view text, bool parallel_policy) const
{
    Query result;
    for (string_view word : SplitIntoWordsView(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }

    if (parallel_policy == 0)
    {
        Deduplicator(result.plus_words);
        Deduplicator(result.minus_words);
    }

    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(string_view word) const
{
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

void MatchDocument(const SearchServer& search_server, string_view raw_query, int document_id)
{
    LOG_DURATION_STREAM("Operation time", std::cout);
    search_server.MatchDocument(raw_query, document_id);
}

void FindTopDocuments(const SearchServer& search_server, string_view raw_query)
{
    LOG_DURATION_STREAM("Operation time", std::cout);
    search_server.FindTopDocuments(raw_query);
}

void AddDocument(SearchServer& search_server, int document_id, string_view raw_query, DocumentStatus status,
    const std::vector<int>& ratings)
{
    LOG_DURATION_STREAM("Operation time", std::cout);
    search_server.AddDocument(document_id, raw_query, status, ratings);
}