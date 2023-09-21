#include "document.h"

using namespace std;

Document::Document() = default;

Document::Document(int id, double relevance, int rating)
    : id(id)
    , relevance(relevance)
    , rating(rating)
{
}

ostream& operator<< (ostream& out, const Document& document)
{
    out << "{ document_id = " << document.id
        << ", relevance = " << document.relevance
        << ", rating = " << document.rating << " }";
    return out;
}