#pragma once

#include <iostream>
#include <cmath>

using std::literals::string_literals::operator""s;

template <typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end) : it_begin_(begin), it_end_(end) {}
    
    Iterator begin() const {
        return it_begin_;
    }
    Iterator end() const {
        return it_end_;
    }
    auto size() const {
        return distance(it_begin_, it_end_);
    }
private:
    Iterator it_begin_;
    Iterator it_end_;
};

template <typename Iterator>
class Paginator {
public:
    Paginator<Iterator> (const Iterator& it_page_begin, const Iterator& it_page_end, size_t page_size) {
        // Узнаем на скольких страницах поместятся все запросы
        size_t number_of_pages = ceil((distance(
            it_page_begin, it_page_end) / static_cast<double>(page_size)));
            
        for (size_t i = 0; i < number_of_pages; ++i) {
            // Разделяем результаты постранично
            if (distance(it_page_begin + page_size * i, it_page_end) > page_size) {
                pages_.push_back(IteratorRange(
                    it_page_begin + page_size * i, it_page_begin + page_size * (i + 1)));
            } else { // Оставшиеся результаты
                pages_.push_back(IteratorRange(it_page_begin + page_size * i, it_page_end));
            }
        }
    }
    
    auto begin() const {
        return pages_.begin();
    }
    auto end() const {
        return pages_.end();
    }
    auto size() const {
        return pages_.size();
    }
    
private:
    std::vector<IteratorRange<Iterator>> pages_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

std::ostream& operator<<(std::ostream& out, const Document& doc) {
    out << "{ document_id = "s << doc.id;
    out << ", relevance = "s << doc.relevance;
    out << ", rating = "s << doc.rating << " }"s;
    return out;
}

template <typename Iterator>
std::ostream& operator<<(std::ostream& out, const IteratorRange<Iterator>& range) {
    for (auto it = range.begin(); it != range.end(); ++it) {
        out << *it;
    }
    return out;
}