#ifndef WEB_TYPES_H
#define WEB_TYPES_H

#include <QUrl>
#include <QByteArray>
#include <QUrlQuery>

typedef struct WebRequest {
    int method;
    QUrl url;
    QUrlQuery query; // decrypted
    QByteArray data;
    QByteArray host;
} WebRequest;

//typedef struct WebResponse {
//    int code;
//    QByteArray data;
//} WebResponse;


inline QString getRandomString(int length)
{
   const QString possibleCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");

   QString randomString;
   for(int i = 0; i < length; ++i)
   {
       int index = qrand() % possibleCharacters.length();
       QChar nextChar = possibleCharacters.at(index);
       randomString.append(nextChar);
   }
   return randomString;
}




#endif // WEB_TYPES_H
