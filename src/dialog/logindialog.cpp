//
// ScreenCloud - An easy to use screenshot sharing application
// Copyright (C) 2016 Olav Sortland Thoresen <olav.s.th@gmail.com>
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later
// version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
// PARTICULAR PURPOSE. See the GNU General Public License for more details.
//

#include "logindialog.h"
#include "ui_logindialog.h"
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    #include <QUrlQuery>
#endif

LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    serverQueryFinished = false;
    serverQueryError = false;
    manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)),
             this, SLOT(replyFinished(QNetworkReply*)));
}

LoginDialog::~LoginDialog()
{
    delete ui;
}
void LoginDialog::on_button_login_clicked()
{
    serverQueryFinished = false;
    serverQueryError = false;
    ui->label_message->setText(tr("Connecting to server..."));
    QString token, tokenSecret;
    QUrl url( "https://api.screencloud.net/1.0/oauth/access_token_xauth" );

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
        QUrlQuery bodyParams;
#else
        QUrl bodyParams;
#endif
    // create body request parameters
    bodyParams.addQueryItem("data[User][email]", ui->input_email->text());
    bodyParams.addQueryItem("data[User][password]", ui->input_password->text());
    bodyParams.addQueryItem("oauth_version", "1.0");
    bodyParams.addQueryItem("oauth_signature_method", "PLAINTEXT");
    bodyParams.addQueryItem("oauth_consumer_key", CONSUMER_KEY_SCREENCLOUD);
    bodyParams.addQueryItem("oauth_signature", CONSUMER_SECRET_SCREENCLOUD);
    bodyParams.addQueryItem("oauth_timestamp", QString::number(QDateTime::currentDateTimeUtc().toTime_t()));
    bodyParams.addQueryItem("oauth_nonce", NetworkUtils::generateNonce(15));
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    QByteArray body = bodyParams.query(QUrl::FullyEncoded).toUtf8();
#else
    QByteArray body = bodyParams.encodedQuery();
#endif

    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    manager->post(request, body);

    while (!serverQueryFinished) {
       qApp->processEvents(QEventLoop::WaitForMoreEvents);
    }
    if(serverQueryError)
    {
        WARNING(url.toString() + tr(" returned error"));
    }else
    {
        this->accept();
    }
}
void LoginDialog::replyFinished(QNetworkReply *reply)
{
    ui->label_message->setText(tr("Reading response from server..."));
    QString replyText = reply->readAll();
    INFO(replyText);
    if(reply->error() != QNetworkReply::NoError)
    {
        serverQueryError = true;
        if(reply->error() == QNetworkReply::ContentAccessDenied || reply->error() == QNetworkReply::AuthenticationRequiredError)
        {
            ui->label_message->setText(tr("<font color='red'>Your email/password combination was incorrect or account is not activated</font>"));
        }else
        {
            ui->label_message->setText(tr("<font color='red'>Login failed. Please check your email, password and internet connection.</font>"));
        }
        WARNING(reply->request().url().toString() + tr(" returned: ") + replyText);

    }else
    {
        //No error in request
        ui->label_message->setText(tr("Logged in..."));
        //Save to qsettings
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
        QUrlQuery replyParams("&" + replyText);
#else
        QUrl replyParams("?" + replyText);
#endif
        QSettings settings("screencloud", "ScreenCloud");
        settings.beginGroup("account");
        settings.setValue("token", replyParams.queryItemValue("oauth_token"));
        settings.setValue("token-secret", replyParams.queryItemValue("oauth_token_secret"));
        settings.setValue("email", ui->input_email->text());
        settings.setValue("logged-in", true);
        settings.endGroup();
    }
    serverQueryFinished = true;

}
