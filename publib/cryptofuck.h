#ifndef CRYPTO_H
#define CRYPTO_H

#include "aes.hpp"
#include <QObject>

// WARNING
// Naive implementation with static keys!!!
// Need to implement session keys initialization

static    uint8_t key[] = { 0x7E, 0x70, 0x43, 0x5D, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x09,
                            0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7, 0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4 };
static    uint8_t iv[]  = { 0x66, 0x40, 0x52, 0x73, 0x50, 0x7E, 0x70, 0x43, 0x5D, 0x5C, 0x44, 0x20, 0x47, 0x6D, 0x53, 0x7A };

inline QString getRandomHexBytes(int length)
{
   const QString possibleCharacters("ABCDEF0123456789");

   QString randomString;
   for(int i = 0; i < length * 2; ++i)
   {
       int index = qrand() % possibleCharacters.length();
       QChar nextChar = possibleCharacters.at(index);
       randomString.append(nextChar);
   }
   return randomString;
}

inline QByteArray encryptByteArray(QByteArray ba_decrypted)
{
    //return ba_decrypted.toBase64(QByteArray::Base64UrlEncoding);

    // USED IN WORKER

    // get BA as data
    // return BA as html response
    // No HEX encoding USED
    struct AES_ctx aes_ctx;

    QByteArray iv_ba;
    for (int i = 0; i < 16; ++i) {
        QByteArray ivb = QByteArray::fromHex(getRandomHexBytes(1).toUtf8());
        iv_ba.append(ivb);
        iv[i]= ivb.at(0);
    }

    AES_init_ctx_iv(&aes_ctx, key, iv);

    QByteArray ba_encrypted = ba_decrypted;
    // make a deep copy bc ba_decrypted is only shallow copy
    // padding
    ba_encrypted.append('F');
    ba_encrypted.append(QByteArray(32 - (ba_encrypted.size() % 32), 'A'));

    // encrypt
    AES_CBC_encrypt_buffer(&aes_ctx, (uint8_t*)(ba_encrypted.data()), ba_encrypted.size());

    // prepend iv
    ba_encrypted.prepend(iv_ba);

    //return ba_encrypted;

    //qDebug() << ba_encrypted.toBase64(QByteArray::Base64UrlEncoding);
    return ba_encrypted.toBase64(QByteArray::Base64UrlEncoding);
}

inline QByteArray decryptByteArray(QByteArray ba_encrypted)
{
    //return QByteArray::fromBase64(ba_encrypted, QByteArray::Base64UrlEncoding);

    // USED IN WORKER

    // get QString as encrypted html get request
    // return decrypted QString
    // HEX encoding USED

    ba_encrypted = QByteArray::fromBase64(ba_encrypted, QByteArray::Base64UrlEncoding);

    if (ba_encrypted.size() < 32) return QByteArray();

    QByteArray iv_ba = ba_encrypted.mid(0, 16);
    ba_encrypted = ba_encrypted.mid(16);

    struct AES_ctx aes_ctx;

    for (int i = 0; i < 16; ++i) {
        iv[i]= iv_ba.at(i);
    }


    AES_init_ctx_iv(&aes_ctx, key, iv);

    QByteArray ba_decrypted = ba_encrypted;
    // make a deep copy bc ba_decrypted is only shallow copy
    ba_decrypted.append('X');
    ba_decrypted.chop(1);

    // decrypt
    AES_CBC_decrypt_buffer(&aes_ctx, (uint8_t*)(ba_decrypted.data()), ba_decrypted.size() + 1);

    // remove padding
    while (ba_decrypted.endsWith('A')) {
        ba_decrypted.chop(1);
    }
    ba_decrypted.chop(1);

    return ba_decrypted;
}



//inline QByteArray decryptFromString(QString string_encrypted)
//{
//    // USED IN WORKER
//    // Base64 encoding USED

//    // get QString as encrypted html get request
//    // return decrypted QByteArray

//    QByteArray ba_encrypted = string_encrypted.toUtf8();
//    ba_encrypted = QByteArray::fromBase64(ba_encrypted, QByteArray::Base64UrlEncoding);

//    return decryptByteArray(ba_encrypted);
//}

//inline QString encryptToString(QByteArray ba_decrypted)
//{
//    // USED IN CLIENT
//    // Base64 encoding USED

//    // get QByteArray as decrypted html get request
//    // return encrypted QString


//    QByteArray ba_encrypted = encryptByteArray(ba_decrypted);

//    return ba_encrypted.toBase64(QByteArray::Base64UrlEncoding);
//}

#endif //CRYPTO_H



