#pragma once
#include<cstddef>
#include<string>
#include<optional>
#include<unordered_map>
#include<vector>
#include"SellerApplicationStore.h"

class InMemorySellerApplicationStore : public SellerApplicationStore
{
    public:
        bool Save(SellerApplication& Application) override;
        std::optional<SellerApplication> FindByID(const std::string& ApplicationID) const override;
        std::optional<SellerApplication> FindByApplicant(const std::string& ApplicantID) const override;
        std::optional<SellerApplication> FindPendingByApplicant(const std::string& ApplicantID) const override;
        bool HasApplication(const std::string& ApplicantID) const override;
        std::vector<SellerApplication> ListByStatus(ApplicationStatus Status) const override;
        std::vector<SellerApplication> All() const override;
        std::size_t Size() const override;

    private:
        std::string MakeNextID();

        std::unordered_map<std::string, SellerApplication> Applications;
        long long LastAssignedNumber = 0;
};
