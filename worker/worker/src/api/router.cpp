#include "router.h"
#include "fuckmacro.h"

//#define PLAINTEXT_API_DEBUG 1

NxResponse onNxRequest(NxRequest request)
{
    static bool isInitialized = false;
    if (!isInitialized) {
        isInitialized = true;
        init();
    }

    //qDebug() << "Request: " << request.method << request.uri;

    //static QMutex mutex;
    //mutex.lock();

    WebRequest webRequest;
    webRequest.method = request.method;
    webRequest.url = QUrl(QString(request.uri));
    webRequest.data = QByteArray(request.data);
    webRequest.host = QByteArray(request.host);


    const QUrlQuery query = QUrlQuery(webRequest.url);
    QString path = webRequest.url.path();

    qDebug() << "Request from " << webRequest.host << " ,raw url: " << path;


    // static requests
    if (staticRequests()->contains(path)) return RESPONSE_200(staticRequests()->value(path).toUtf8());

    path.remove(0,1);

    if (path.startsWith(API_VERSION_VALUE)) {
        path = path.mid(QString(API_VERSION_VALUE).size());
    }
    else {
        qDebug() << "Wrong API handler:" << path;
        return RESPONSE_404();
    }

    path.remove(0,1);
    //qDebug() << "Request processed path: " << path;

    QString decrypted_path = QString(decryptByteArray(path.toUtf8()));

#ifdef PLAINTEXT_API_DEBUG
    if (path.size() > 2 && path.at(1) == '_') {
        decrypted_path = path;
    }
#endif

    qDebug() << "Request decoded query: " << decrypted_path;

    if (decrypted_path.contains("?")) {
        webRequest.query = QUrlQuery(decrypted_path.split("?").at(1));
    }
    else {
        webRequest.query = QUrlQuery();
    }
    qDebug() << "Request processed query: " << webRequest.query.toString(QUrl::FullyDecoded);


    if (decrypted_path.startsWith("c_test")) return c_test(webRequest);
    if (decrypted_path.startsWith("c_getServers")) return c_getServers(webRequest);
    if (decrypted_path.startsWith("c_getOvpnFile")) return c_getOvpnFile(webRequest);
    if (decrypted_path.startsWith("c_checkIfUserExists")) return c_checkIfUserExists(webRequest);
    if (decrypted_path.startsWith("c_auth")) return c_auth(webRequest);
    if (decrypted_path.startsWith("c_activateKey")) return c_activateKey(webRequest);

    if (decrypted_path.startsWith("m_reloadJsons")) return m_reloadJsons(webRequest);



    // add user when he pay
    //if (path == "/m_addUser") return m_addUser(webRequest);


    return RESPONSE_404();
}

