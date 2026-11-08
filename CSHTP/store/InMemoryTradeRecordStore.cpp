#include"InMemoryTradeRecordStore.h"
#include<algorithm>

namespace
{
    // 一条记录是否满足全部非空条件（取「与」）
    bool Matches(const TradeRecord& Record, const TradeQuery& Cond)
    {
        // 精确匹配的几个字段
        if (Cond.TradeID.has_value() && Record.TradeID != Cond.TradeID.value())
        {
            return false;
        }

        if (Cond.ItemID.has_value() && Record.ItemID != Cond.ItemID.value())
        {
            return false;
        }

        if (Cond.BuyerID.has_value() && Record.BuyerID != Cond.BuyerID.value())
        {
            return false;
        }

        if (Cond.SellerID.has_value() && Record.SellerID != Cond.SellerID.value())
        {
            return false;
        }

        // 「我相关的成交」= 买家是我 或 卖家是我。
        // 条件本身为空串时一律不命中：库里不存在空 ID，不该被误当成「不限」。
        if (Cond.ParticipantID.has_value())
        {
            const std::string& Who = Cond.ParticipantID.value();
            if (Who.empty() || (Record.BuyerID != Who && Record.SellerID != Who))
            {
                return false;
            }
        }

        // 模糊匹配：子串出现即可
        if (Cond.Keyword.has_value()
            && Record.ItemTitle.find(Cond.Keyword.value()) == std::string::npos)
        {
            return false;
        }

        if (Cond.TradeLocation.has_value()
            && Record.TradeLocation.find(Cond.TradeLocation.value()) == std::string::npos)
        {
            return false;
        }

        // 数值区间：两端都包含
        if (Cond.MinPriceCents.has_value() && Record.PriceCents < Cond.MinPriceCents.value())
        {
            return false;
        }

        if (Cond.MaxPriceCents.has_value() && Record.PriceCents > Cond.MaxPriceCents.value())
        {
            return false;
        }

        if (Cond.TradedAfterMs.has_value() && Record.TradedAtMs < Cond.TradedAfterMs.value())
        {
            return false;
        }

        if (Cond.TradedBeforeMs.has_value() && Record.TradedAtMs > Cond.TradedBeforeMs.value())
        {
            return false;
        }

        return true;
    }

    // 排序用的比较函数。
    // 主键相同时用 TradeID 兜底，否则同一时刻的多条记录谁在前取决于
    // 哈希表的遍历顺序——Libstdc++ 恰好稳定，别的实现不保证，测试会飘。
    bool LessByNewest(const TradeRecord& A, const TradeRecord& B)
    {
        if (A.TradedAtMs != B.TradedAtMs)
        {
            return A.TradedAtMs > B.TradedAtMs;
        }
        return A.TradeID < B.TradeID;
    }

    bool LessByOldest(const TradeRecord& A, const TradeRecord& B)
    {
        if (A.TradedAtMs != B.TradedAtMs)
        {
            return A.TradedAtMs < B.TradedAtMs;
        }
        return A.TradeID < B.TradeID;
    }

    bool LessByPriceAsc(const TradeRecord& A, const TradeRecord& B)
    {
        if (A.PriceCents != B.PriceCents)
        {
            return A.PriceCents < B.PriceCents;
        }
        return LessByOldest(A, B);
    }

    bool LessByPriceDesc(const TradeRecord& A, const TradeRecord& B)
    {
        if (A.PriceCents != B.PriceCents)
        {
            return A.PriceCents > B.PriceCents;
        }
        return LessByOldest(A, B);
    }

    void SortRecords(std::vector<TradeRecord>& Out, TradeSort Sort)
    {
        switch (Sort)
        {
            case TradeSort::TradedNewest:
                std::sort(Out.begin(), Out.end(), LessByNewest);
                break;

            case TradeSort::TradedOldest:
                std::sort(Out.begin(), Out.end(), LessByOldest);
                break;

            case TradeSort::PriceAsc:
                std::sort(Out.begin(), Out.end(), LessByPriceAsc);
                break;

            case TradeSort::PriceDesc:
                std::sort(Out.begin(), Out.end(), LessByPriceDesc);
                break;

            case TradeSort::None:
            default:
                // 不排序：按哈希表的遍历顺序，只适合测试里比较集合，
                // 不适合直接渲染
                break;
        }
    }
}  // namespace

bool InMemoryTradeRecordStore::Save(TradeRecord& Record)
{
    // 没有编号 = 新记录，由存储层分配一个
    if (Record.TradeID.empty())
    {
        Record.TradeID = MakeNextID();
    }

    // 有编号 = 新增或覆盖
    Records.insert_or_assign(Record.TradeID, Record);
    return true;
}

std::optional<TradeRecord> InMemoryTradeRecordStore::FindByID(const std::string& TradeID) const
{
    const auto It = Records.find(TradeID);
    if (It == Records.end())
    {
        return std::nullopt;
    }
    return It->second;
}

std::vector<TradeRecord> InMemoryTradeRecordStore::Query(const TradeQuery& Cond) const
{
    std::vector<TradeRecord> Out;
    Out.reserve(Records.size());

    for (const auto& Entry : Records)
    {
        if (Matches(Entry.second, Cond))
        {
            Out.push_back(Entry.second);
        }
    }

    // 先筛后排：排序只作用在结果上，不改动存储本身
    SortRecords(Out, Cond.Sort);
    return Out;
}

std::vector<TradeRecord> InMemoryTradeRecordStore::All() const
{
    // 空条件 = 全部；默认排序就是从新到旧
    return Query(TradeQuery{});
}

std::size_t InMemoryTradeRecordStore::Size() const
{
    return Records.size();
}

std::string InMemoryTradeRecordStore::MakeNextID()
{
    // 循环是为了防御「编号已被占用」（比如以后从文件读入数据）
    while (true)
    {
        ++LastAssignedNumber;
        const std::string Candidate = "trade-" + std::to_string(LastAssignedNumber);
        if (Records.find(Candidate) == Records.end())
        {
            return Candidate;
        }
    }
}
