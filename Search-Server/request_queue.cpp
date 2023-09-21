#include "request_queue.h"

using namespace std;

RequestQueue::RequestQueue(const SearchServer& search_server)
    : data_base_(search_server)
{
}

vector<Document> RequestQueue::AddFindRequest(string_view raw_query, DocumentStatus status)
{
    return AddFindRequest(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
        });
}

vector<Document> RequestQueue::AddFindRequest(string_view raw_query)
{
    return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
}

int RequestQueue::GetNoResultRequests() const
{
    return empty_count_;
}

