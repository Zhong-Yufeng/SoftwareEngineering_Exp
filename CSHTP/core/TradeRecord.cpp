#include"TradeRecord.h"

// ============================================================
// 成交记录
//
// 这里只校验「这条记录本身能不能成立」。商品在不在库里、预约有没有被
// 确认过、调用者是不是这个卖家，都要先查库才知道，属于服务层的编排。
// ============================================================
std::optional<TradeRecord> TradeRecord::Create(const Item& Goods,
                                               const std::string& BuyerID,
                                               const std::string& BuyerContact,
                                               long long TradedAtMs)
{
    // 没有编号的商品还没入库，不能拿来成交
    if (Goods.GetItemID().empty() || Goods.GetSellerID().empty())
    {
        return std::nullopt;
    }

    // 只有已发布（在售）的商品能成交。
    // 草稿、驳回、待审核都不是「可供购买」，放行的话，一份没通过审核的
    // 内容也能变成正式成交记录。
    if (Goods.GetStatus() != ItemStatus::Published)
    {
        return std::nullopt;
    }

    // 买家必须有身份，且不能买自己的东西
    if (BuyerID.empty() || BuyerID == Goods.GetSellerID())
    {
        return std::nullopt;
    }

    // 双方联系方式是成交后唯一的联系途径，缺一个这条记录就残缺。
    // 成交后商品和预约都会被删除，没有别的地方能补。
    if (BuyerContact.empty() || Goods.GetContact().empty())
    {
        return std::nullopt;
    }

    // 价格为负没有业务含义。0 元商品在 Item::CreateDraft 就被挡掉了，
    // 这里是防御性检查，不承担主要校验职责。
    if (Goods.GetPriceCents() < 0)
    {
        return std::nullopt;
    }

    // 成交时间由调用方传入而不是内部读时钟，便于测试断言具体数值
    if (TradedAtMs <= 0)
    {
        return std::nullopt;
    }

    TradeRecord Record;

    Record.TradeID = "";                    // 编号留给存储层分配
    Record.ItemID = Goods.GetItemID();
    Record.BuyerID = BuyerID;
    Record.SellerID = Goods.GetSellerID();
    Record.TradedAtMs = TradedAtMs;

    // 以下五个字段是「成交那一刻的快照」：拷的是值，不是指向商品的引用。
    // 商品随后会被删除，卖家日后也会改联系方式，这条记录都不应跟着变。
    Record.ItemTitle = Goods.GetTitle();
    Record.PriceCents = Goods.GetPriceCents();
    Record.TradeLocation = Goods.GetTradeLocation();
    Record.SellerContact = Goods.GetContact();
    Record.BuyerContact = BuyerContact;

    return Record;
}

bool TradeRecord::IsParticipant(const std::string& UserID) const
{
    // 空串必须挡掉：记录里不会出现空 ID，但调用方误传空串时，
    // 不能因为「空 == 空」把陌生人当成参与者。
    if (UserID.empty())
    {
        return false;
    }
    return UserID == BuyerID || UserID == SellerID;
}
