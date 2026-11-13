#pragma once
#include<cstddef>
#include<optional>
#include<string>
#include<vector>
#include"ServiceCommon.h"
#include"User.h"
#include"Item.h"
#include"UserStore.h"
#include"ItemStore.h"
#include"TradeRecordStore.h"

// 按角色拆分的服务类共用的权限判定、可见性过滤与级联动作。
// 这里只放「与具体用例无关」的规则，避免每个服务类各抄一份。
namespace ServiceRules
{
    // 封禁提示带上原因（REQ-017），无原因时用统一文案
    inline std::string BannedMessage(const User& Target)
    {
        if (Target.Reason.has_value())
        {
            return "账号已被封禁：" + Target.Reason.value();
        }
        return "账号已被封禁";
    }

    // 调用者必须是处于正常状态的买家（未封禁、非管理员）
    inline bool RequireBuyer(UserStore& Users, const std::string& BuyerID, std::string* Error)
    {
        const auto Buyer = Users.FindByID(BuyerID);
        if (!Buyer.has_value())
        {
            SetError(Error, "用户不存在");
            return false;
        }
        if (Buyer->Status == AccountStatus::Banned)
        {
            SetError(Error, "账号已被封禁");
            return false;
        }
        if (Buyer->UserRole == Role::Admin)
        {
            SetError(Error, "管理员不能发起预约");
            return false;
        }
        return true;
    }

    // 调用者必须是处于正常状态的卖家
    inline bool RequireSeller(UserStore& Users, const std::string& SellerID, std::string* Error)
    {
        const auto Seller = Users.FindByID(SellerID);
        if (!Seller.has_value())
        {
            SetError(Error, "用户不存在");
            return false;
        }
        if (Seller->Status == AccountStatus::Banned)
        {
            SetError(Error, "账号已被封禁");
            return false;
        }
        if (Seller->UserRole != Role::Seller)
        {
            SetError(Error, "仅卖家可执行该操作");
            return false;
        }
        return true;
    }

    // 按调用者身份取一条商品，不可见时提示「无权查看该商品」
    inline std::optional<Item> FindVisibleItem(ItemStore& Items,
                                               Role ViewerRole,
                                               const std::string& ViewerID,
                                               const std::string& ItemID,
                                               std::string* Error)
    {
        ClearError(Error);

        const auto Goods = Items.FindByID(ItemID);
        if (!Goods.has_value())
        {
            SetError(Error, "商品不存在");
            return std::nullopt;
        }
        if (!Goods->IsVisibleTo(ViewerRole, ViewerID))
        {
            SetError(Error, "无权查看该商品");
            return std::nullopt;
        }
        return Goods;
    }

    // 按调用者身份过滤查询结果（不可见的商品直接跳过，不报错）
    inline std::vector<Item> QueryVisibleItems(ItemStore& Items,
                                               const ItemQuery& Cond,
                                               Role ViewerRole,
                                               const std::string& ViewerID)
    {
        std::vector<Item> Result;
        for (const Item& Goods : Items.Query(Cond))
        {
            if (Goods.IsVisibleTo(ViewerRole, ViewerID))
            {
                Result.push_back(Goods);
            }
        }
        return Result;
    }

    // 商品失效（下架 / 违规下架 / 卖家封禁）时打断进行中的交易记录：
    // 记录保留但转为「已作废」，买家仍能查到约成过什么、为什么作废。
    inline std::size_t CancelTradesByItem(TradeRecordStore& Trades,
                                          const std::string& ItemID,
                                          const std::string& Reason,
                                          long long NowMs)
    {
        if (ItemID.empty() || Reason.empty())
        {
            return 0;
        }

        std::size_t CancelledCount = 0;
        for (const TradeRecord& Trade : Trades.ListByItem(ItemID))
        {
            // 已完成的交易已经发生，已作废的也已处理过，只有进行中的需要打断
            if (Trade.Status != TradeStatus::InProgress)
            {
                continue;
            }

            TradeRecord Target = Trade;
            if (!Target.Cancel(Reason, NowMs))
            {
                continue;
            }
            if (!Trades.Save(Target))
            {
                continue;
            }
            ++CancelledCount;
        }
        return CancelledCount;
    }
}
