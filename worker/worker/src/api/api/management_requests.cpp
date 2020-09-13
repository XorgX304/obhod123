#include "management_requests.h"



/// Authorization


/// Response OK 200 everytime


NxResponse m_reloadJsons(WebRequest request)
{
    // Public
    QString user_id = request.query.queryItemValue("user_id");
    if (user_id != "some_admin_password_or_secret") return RESPONSE_400();

    loadUsersJson();
    loadServersJson();
    loadStaticRequests();

    return RESPONSE_200("ok");
}
