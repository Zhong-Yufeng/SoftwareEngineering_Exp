#pragma once
#include<optional>
#include<vector>
#include<string>
#include<cstddef>
#include"SellerApplication.h"

// 一个申请人最多一条记录，只增不删
class SellerApplicationStore
{
    public:
        virtual ~SellerApplicationStore() = default;

        virtual bool Save(SellerApplication& Application) = 0;
        virtual std::optional<SellerApplication> FindByID(const std::string& ApplicationID) const = 0;
        virtual std::optional<SellerApplication> FindByApplicant(const std::string& ApplicantID) const = 0;
        virtual std::optional<SellerApplication> FindPendingByApplicant(const std::string& ApplicantID) const = 0;
        virtual bool HasApplication(const std::string& ApplicantID) const = 0;
        virtual std::vector<SellerApplication> ListByStatus(ApplicationStatus Status) const = 0;
        virtual std::vector<SellerApplication> All() const = 0;
        virtual std::size_t Size() const = 0;
};
