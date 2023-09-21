#pragma once

#include <iostream>
#include <vector>

template<class RandomIt>
class IteratorRange
{
public:

    explicit IteratorRange(const RandomIt begin, const RandomIt end)
        : begin_(begin), end_(end)
    {
    }

    RandomIt begin() const
    {
        return begin_;
    }

    RandomIt end() const
    {
        return end_;
    }

    size_t size() const
    {
        return distance(begin_, end_);
    }

private:
    RandomIt begin_;
    RandomIt end_;
};

template<class RandomIt>
std::ostream& operator<<(std::ostream& out, const IteratorRange<RandomIt>& itr)
{
    for (auto it = itr.begin(); it != itr.end(); advance(it, 1))
    {
        out << *it;
    }
    return out;
}

template<typename RandomIt>
class Paginator
{
public:


    explicit Paginator(RandomIt begin, RandomIt end, int page_size) :
        page_size_(page_size)
    {
        PaginatorConstruct(begin, end);
    }

    auto begin() const
    {
        return pages_.begin();
    }

    auto end() const
    {
        return pages_.end();
    }

    size_t size() const
    {
        return pages_.size();
    }

private:


    void PaginatorConstruct(const RandomIt begin, RandomIt end)
    {
        auto it_beg = begin, it_end = begin;


        size_t page_size = page_size_;
        while (it_end != end)
        {
            if (distance(it_beg, end) < page_size)
            {
                page_size = distance(it_beg, end);
            }

            advance(it_end, page_size);
            pages_.push_back(IteratorRange(it_beg, it_end));
            advance(it_beg, page_size);
        }

    }

    std::vector<IteratorRange<RandomIt>> pages_;
    size_t page_size_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}