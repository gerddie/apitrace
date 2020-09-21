#include "ft_frametrimmer.hpp"

#include <unordered_set>

namespace frametrim {

struct CFrameTrimmeImpl {
    CFrameTrimmeImpl();
    void call(const trace::Call& call, bool in_target_frame);
    std::vector<unsigned> get_sorted_call_ids() const;



};

CFrameTrimmer::CFrameTrimmer()
{
    impl = new CFrameTrimmeImpl;
}

CFrameTrimmer::~CFrameTrimmer()
{
    delete impl;
}

void
CFrameTrimmer::call(const trace::Call& call, bool in_target_frame)
{

}

std::vector<unsigned>
CFrameTrimmer::get_sorted_call_ids() const
{
    return impl->get_sorted_call_ids();
}



CFrameTrimmeImpl::CFrameTrimmeImpl()
{

}

void
CFrameTrimmeImpl::call(const trace::Call& call, bool in_target_frame)
{

}

std::vector<unsigned>
CFrameTrimmeImpl::get_sorted_call_ids() const
{
    std::unordered_set<unsigned> make_sure_its_singular;

    for(auto&& c: m_required_calls)
        make_sure_its_singular.insert(c->call_no());

    std::vector<unsigned> sorted_calls(
                make_sure_its_singular.begin(),
                make_sure_its_singular.end());
    std::sort(sorted_calls.begin(), sorted_calls.end());
    return sorted_calls;
}



}
