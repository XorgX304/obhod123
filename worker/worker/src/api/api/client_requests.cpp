#include "client_requests.h"


NxResponse c_getServers(WebRequest request)
{
    // Public
    Q_UNUSED(request);
    return RESPONSE_200(QJsonDocument(*serversJson()).toJson());
}

NxResponse c_getOvpnFile(WebRequest request)
{
    QMutexLocker mutexLocker(usersJsonMutex());

    // Private
    // Requires client username and password in json

    // testing with GET

    QString user_id = request.query.queryItemValue("user_id");

    const QJsonArray &users = usersJson()->value("users").toArray();
    for (int i = 0; i < users.size(); ++i) {
        const QJsonObject &user = users.at(i).toObject();
        if (user.value("id").toString() == user_id
                && QDateTime::fromTime_t(user.value("valid").toString().toUInt()) > QDateTime::currentDateTimeUtc()) {
            QString rnd = getRandomString(32);
            d_addUser(rnd);
            QString ovpn = d_getUser(rnd);

            QJsonObject log;
            log.insert("user_id", user_id);
            log.insert("time", QString::number(QDateTime::currentDateTimeUtc().toTime_t()));
            log.insert("ip", QString(request.host));
            connectionLog()->append(log);

            return RESPONSE_200(ovpn.toUtf8());
        }
    }

    //qDebug() << "User not found" << userName << pw;
    return RESPONSE_400();
}

NxResponse c_checkIfUserExists(WebRequest request)
{
    QMutexLocker mutexLocker(usersJsonMutex());

    // Public
    QString user_id = request.query.queryItemValue("user_id");

    const QJsonArray &users = usersJson()->value("users").toArray();
    for (int i = 0; i < users.size(); ++i) {
        if (users.at(i).toObject().value("id").toString() == user_id) {
            return RESPONSE_200(QString("1").toUtf8());
        }
    }
    return RESPONSE_200(QString("0").toUtf8());
}

//NxResponse m_addUser(WebRequest request)
//{
//    // Public
//    QJsonObject newUser = QJsonDocument::fromJson(request.data).object();

//    QString userName = newUser.value("name").toString();
//    if (newUser.value("name").toString().isEmpty()) return RESPONSE_400();

//    QJsonArray users = usersJson()->value("users").toArray();
//    for (int i = 0; i < users.size(); ++i) {
//        if (users.at(i).toObject().value("name").toString() == userName) {
//            return RESPONSE_200(QString("0").toUtf8());
//        }
//    }

//    users.append(newUser);;

//    usersJson()->insert("users", users);
//    saveUsersJson();

//    return RESPONSE_200(QString("1").toUtf8());
//}


NxResponse c_test(WebRequest request)
{
    return RESPONSE_400();


//    QProcess p;
//    p.setProcessChannelMode(QProcess::MergedChannels);

//    p.start("sh", QStringList()
//            << "/obhod123/test.sh"
//            << "aaa"
//            << "--rm");

//    bool isStarted = p.waitForStarted(1000);
//    if (isStarted) qDebug() << "test p started";
//    else qDebug() << p.errorString();


//    p.waitForFinished();
//    QString output = QString(p.readAll());

//    return RESPONSE_200(output.toUtf8());

}

NxResponse c_auth(WebRequest request)
{
    QMutexLocker mutexLocker(usersJsonMutex());

    // Public
    QString user_id = request.query.queryItemValue("user_id");

    QJsonObject response;

    const QJsonArray &users = usersJson()->value("users").toArray();
    for (int i = 0; i < users.size(); ++i) {
        QJsonObject user = users.at(i).toObject();
        if (user.value("id").toString() == user_id) {
            if (QDateTime::fromTime_t(user.value("valid").toString().toUInt()) > QDateTime::currentDateTimeUtc()) {
                response.insert("response", "AUTH_SUCCESS");
            }
            else {
                response.insert("response", "AUTH_KEY_EXPIRED");
            }
            response.insert("valid", user.value("valid").toString());
            response.insert("mode", user.value("mode").toString());
            return RESPONSE_200(QJsonDocument(response).toJson());
        }
    }

    response.insert("response", "AUTH_KEY_NOT_FOUND");
    return RESPONSE_200(QJsonDocument(response).toJson());
}

//NxResponse c_newOrder_Fondy(WebRequest request)
//{
//    // request example
////    {
////      "request":{
////        "order_id":"test123456",
////        "order_desc":"test order",
////        "currency":"USD",
////        "amount":"125",
////        "signature":"f0ee6288b9295d3b808bcd8d720211c7201245e1",
////        "merchant_id":"1396424"
////      }
////    }


//    // Client send his user_id=256bit key
//    // Server respond with new order_id, checkout_url
//    // Client open checkout_url

//    QString user_id = request.query.queryItemValue("user_id");
//    QString order_id = QString::number(QDateTime::currentDateTimeUtc().toTime_t()) + getRandomString(8);
//    QString server_ip = QProcessEnvironment::systemEnvironment().value("ip");
//    QString server_callback_url = "http://" + server_ip + ":8080/" + API_VERSION_VALUE + "/callback_fondy";
//    QString merchant_password = "nt7UUe044FkN2Efm7fS89fs6j3Q4TjCj";

//    // Insert to orders

//    QJsonObject r;
//    r.insert("order_id", order_id);
//    r.insert("order_desc", "30 days access to the service");
//    r.insert("currency", "USD");
//    r.insert("amount", "100");
//    r.insert("merchant_id", "1409403");
//    r.insert("product_id", "1");
//    r.insert("subscription", "Y");
//    r.insert("lang", "ru");
//    r.insert("lifetime", "3600");
//    r.insert("server_callback_url", server_callback_url);


//    r.insert("signature", calculateFondyHash(r));

//}

//QString calculateFondyHash(const QJsonObject &o)
//{
//    QList<QPair<QString, QString>> list;
//    for (int i = 0; i < o.keys().size(); ++i) {
//        QString key = o.keys().at(i);
//        if (! o.value(key).toString().isEmpty()) {
//            list.append(qMakePair<QString, QString>(key, o.value(key).toString()));
//        }
//    }

//    // sort list in alphabetical
//    qSort(list);

//    // concat values
//    QStringList valuesList;

//    for (int i = 0; i < list.size(); ++i) {
//        valuesList.append(list.at(i).second);
//    }

//    QString valuesString = valuesList.join("|");

//    QString hash = QCryptographicHash::hash(valuesString.toLatin1(), QCryptographicHash::Sha1);
//    return hash;
//}

NxResponse c_activateKey(WebRequest request)
{
    QMutexLocker mutexLocker(usersJsonMutex());

    // Public
    QString new_user_id = request.query.queryItemValue("new_user_id"); // new key entered to lineedit
    QString old_user_id = request.query.queryItemValue("old_user_id"); // old key saved in settings

    QJsonObject response;

    QJsonObject newUser;
    QJsonObject oldUser;

    const QJsonArray &users = usersJson()->value("users").toArray();
    int oldUserIndex = -1;;
    int newUserIndex = -1;;

    for (int i = 0; i < users.size(); ++i) {
        QJsonObject user = users.at(i).toObject();
        if (user.value("id").toString() == new_user_id) {
            newUser = user;
            newUserIndex = i;
        }
        if (old_user_id != "free" && user.value("id").toString() == old_user_id) {
            oldUser = user;
            oldUserIndex = i;
        }
    }



    if (newUser.value("id").toString().isEmpty()) {
        response.insert("response", "ACTIVATION_KEY_NOT_FOUND");
        return RESPONSE_200(QJsonDocument(response).toJson());
    }


    QDateTime overallValid;
    QDateTime oldValid = QDateTime::fromTime_t(oldUser.value("valid").toString().toUInt());


    // check if new key mode is >= than the old
    // otherwise dont add days to old key
    bool allowToAddDays = false;
    QString oldMode = oldUser.value("mode").toString();
    QString newMode = newUser.value("mode").toString();

    if (oldMode == "basic" && (newMode == "basic" || newMode == "pro" || newMode == "premium")) allowToAddDays = true;
    if (oldMode == "pro"   && (newMode == "pro" || newMode == "premium")) allowToAddDays = true;
    if (oldMode == "premium" && (newMode == "premium")) allowToAddDays = true;


    // if no oldUser lets assume that the old license experied right now
    if (!oldUser.value("id").toString().isEmpty()) {
        if (oldValid > QDateTime::currentDateTimeUtc()) overallValid = oldValid;
        else overallValid = QDateTime::currentDateTimeUtc();
    }
    else {
        overallValid = QDateTime::currentDateTimeUtc();
    }

    // new generated user should contain field "valid" with value add_30, add_365, add_N days
    bool isNewUserActivated = ! newUser.value("valid").toString().startsWith("add_");

    //
    uint newUserAddDays = 0;
    if (! isNewUserActivated) {
        QString add_days = newUser.value("valid").toString();
        add_days.replace("add_", "");
        newUserAddDays = add_days.toUInt();
    }


    // return ACTIVATION_KEY_EXPIRED if new key already activated and expired
    if (isNewUserActivated && newUser.value("valid").toString().toUInt() < QDateTime::currentDateTimeUtc().toTime_t()) {
        response.insert("response", "ACTIVATION_KEY_EXPIRED");
//        response.insert("isNewUserActivated", isNewUserActivated);
//        response.insert("newUser", newUser);
//        response.insert("currentDateTimeUtc", newUser);
        return RESPONSE_200(QJsonDocument(response).toJson());
    }


    if (isNewUserActivated) {
        response.insert("valid", newUser.value("valid").toString());
    }
    else {
        if (allowToAddDays && newUserAddDays > 0) {
            overallValid.addDays(newUserAddDays);

            // save new user as we added days
            // TODO rewrite this shit
            newUser.insert("valid", QString::number(overallValid.toTime_t()));
            QJsonArray users = usersJson()->value("users").toArray();
            users.replace(newUserIndex, newUser);
        }
        response.insert("valid", QString::number(overallValid.toTime_t()));
    }
    response.insert("mode", newUser.value("mode").toString());
    response.insert("response", "ACTIVATION_SUCCESS");
    return RESPONSE_200(QJsonDocument(response).toJson());



//    if (oldUser.value("id").toString().isEmpty()) {
//        response.insert("response", "ACTIVATION_SUCCESS");
//        response.insert("valid", newUser.value("valid").toString());
//        response.insert("mode", newUser.value("mode").toString());
//        return RESPONSE_200(QJsonDocument(response).toJson());
//    }
//    else {
//        // TODO: add days to old key

//        response.insert("response", "ACTIVATION_SUCCESS");
//        response.insert("valid", oldUser.value("valid").toString());
//        response.insert("mode", oldUser.value("mode").toString());
//        return RESPONSE_200(QJsonDocument(response).toJson());
//    }




}
