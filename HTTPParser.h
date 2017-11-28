// HTTPParser.h

#ifndef _HTTPPARSER_H
#define _HTTPPARSER_H

typedef struct {
    string Type, path, version, connection, host;
} HttpReq_Header;

typedef struct {
    string version, status_code, status_string;
    string contType, contLength, connection;
} HttpResp_Header;

string get_mime_type(string& ext);
extern string HttpResp_Builder(HttpResp_Header* Resp_Header);
extern bool HttpReq_Parser(HttpReq_Header* ReqHeader, string request);

#endif /* _HTTPPARSER_H */
