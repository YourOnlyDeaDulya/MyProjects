#include "process_queries.h"

#include <functional>

using namespace std;

vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const vector<string>& queries)
{
    vector<vector<Document>> found_docs(queries.size());
    transform(execution::par, queries.begin(), queries.end(),
        found_docs.begin(),
        [&](const string& query) { return search_server.FindTopDocuments(query); });

    return found_docs;
}

list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const vector<string>& queries)
{
    auto found = ProcessQueries(search_server, queries);

    list<Document> found_docs_with_info;
    size_t size = 0;
    for_each(found.begin(), found.end(),
        [&](const vector<Document>& doc)
        {
            found_docs_with_info.resize(found_docs_with_info.size() + doc.size());
            auto it = next(found_docs_with_info.begin(), size);
            transform(execution::par, doc.begin(), doc.end(), it,
                [](const Document& document)
                {
                    return document;
                });
            size += doc.size();
        });
    return found_docs_with_info;
}