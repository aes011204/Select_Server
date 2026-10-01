#include "UserManager.h"
namespace NLogicLib
{
    Protocol::LoginResult UserManager::Login(NServerNetLib::SessionId sessionId, const std::string& nickname)
    {
        // 같은 연결에서 두번 로그인 하는지 확인 
        if (m_users.find(sessionId) != m_users.end())
        {
            return Protocol::LoginResult::AlreadyLoggedIn;
        }

        if (!IsValidNickname(nickname))
        {
            return Protocol::LoginResult::InvalidNickname;
        }

        if (m_sessionByNickname.find(nickname) != m_sessionByNickname.end())
        {
            return Protocol::LoginResult::NicknameInUse;
        }

        User user;
        user.Session = sessionId;
        user.Nickname = nickname;

        m_users.emplace(sessionId, user);
        m_sessionByNickname.emplace(nickname, sessionId);

        return Protocol::LoginResult::Success;

    }

    const User* NLogicLib::UserManager::Find(NServerNetLib::SessionId sessionId) const
    {
        const auto  iter = m_users.find(sessionId);

        if (iter == m_users.end())
        {
            return nullptr;
        }

        return &iter->second;
    }

    void NLogicLib::UserManager::Remove(NServerNetLib::SessionId sessionId)
    {
        const auto iter = m_users.find(sessionId);

        if (iter == m_users.end())
        {
            return;
        }

        m_sessionByNickname.erase(iter->second.Nickname);
        m_users.erase(iter);
    }

    std::vector<NServerNetLib::SessionId> UserManager::GetSessionId() const
    {
        // 사용자 맵을 직접 공개하지 않고 ID 목록을 복사해서 줌
        std::vector<NServerNetLib::SessionId> result;
        result.reserve(m_users.size());

        for (const auto& entry : m_users)
        {
            result.push_back(entry.first);
        }

        return result;
    }

    bool NLogicLib::UserManager::IsValidNickname(const std::string& nickname) const
    {
        if (nickname.empty() || nickname.size() > Protocol::MAX_NICKNAME_BYTES)
        {
            return false;
        }

        for (unsigned char ch : nickname)
        {
            const bool allowed = (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
                (ch >= '0' && ch <= '9') || ch == '_';

            if (!allowed)
            {
                return false;
            }
        }

        return true;
    }
    User* UserManager::FindMutable(NServerNetLib::SessionId sessionId)
    {
        const auto it = m_users.find(sessionId);

        if (it == m_users.end())
        {
            return nullptr;
        }

        return &it->second;
    }
}