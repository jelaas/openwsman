//----------------------------------------------------------------------------
//
//  Copyright (C) Intel Corporation, 2007.
//
//  File:       OpenWsmanClient.cpp
//
//  Contents:   An implementation of the WsmanClient interface using openwsman
//
//----------------------------------------------------------------------------

#include "OpenWsmanClient.h"


extern "C" {
#include "wsman-api.h"
}

#include "wsman-client-transport.h"

#define WSMAN_ENCODING		"UTF-8"

using namespace WsmanClientNamespace;

static bool CheckWsmanResponse(WsManClient* cl, WsXmlDocH& doc);
static bool ResourceNotFound(WsManClient* cl, WsXmlDocH& enumerationRes);
static string XmlDocToString(WsXmlDocH& doc);
static client_opt_t *SetDefaultOptions();
static string ExtractPayload(WsXmlDocH& doc);
static string ExtractItems(WsXmlDocH& doc);

// Construct from params.

OpenWsmanClient::OpenWsmanClient(const char *host,
				 const int port,
				 const char *path ,
				 const char *scheme,
				 const char *auth_method ,
				 const char *username,
				 const char *password
#ifdef _WIN32
				 // determines which cert store to search
				 ,const bool local,
				 // search for a client cert with this name
				 const char *cert,
				 // search for a cient cert with this oid
				 const char *oid
#endif
				 )
{
	cl = wsmc_create(host, port, path, scheme, username, password);
	wsmc_transport_init(cl, (void*)NULL);
	SetAuth(auth_method);	
#ifdef _WIN32
	SetClientCert(oid, cert, local);
#endif
}

// Destructor.
OpenWsmanClient::~OpenWsmanClient() 
{
	wsmc_transport_fini(cl);
	wsmc_release(cl);
}

string OpenWsmanClient::Create(const string &resourceUri, const string &data) const
{
	client_opt_t *options = NULL;
	options = SetDefaultOptions();
	WsXmlDocH createResponse = wsmc_action_create_fromtext(cl, 
			resourceUri.c_str(),
			options,
			data.c_str(), data.length(), WSMAN_ENCODING);
	wsmc_options_destroy(options);
	CheckWsmanResponse(cl, createResponse);
	string xml = ExtractPayload(createResponse);
	ws_xml_destroy_doc(createResponse);
	return xml; 
}

void OpenWsmanClient::Delete(const string &resourceUri, const NameValuePairs *s) const
{
	client_opt_t *options;
	options = SetDefaultOptions();
	if(s)
	{
		// Add selectors.
		for (PairsIterator p = s->begin(); p != s->end(); ++p) {
			if(p->second != "")
				wsmc_add_selector(options, 
						(char *)p->first.c_str(), (char *)p->second.c_str());
		}
	}
	WsXmlDocH deleteResponse = wsmc_action_delete(	cl, 
			(char *)resourceUri.c_str(),
			options);
	wsmc_options_destroy(options);
	CheckWsmanResponse(cl, deleteResponse);
	ws_xml_destroy_doc(deleteResponse);
}

void OpenWsmanClient::Enumerate(const string &resourceUri, vector<string> &enumRes, const NameValuePairs *s) const
{
	client_opt_t *options = NULL;
	options = SetDefaultOptions();
	if(s)
	{
		// Add selectors.
		for (PairsIterator p = s->begin(); p != s->end(); ++p) {
			if(p->second != "")
				wsmc_add_selector(options, 
						(char *)p->first.c_str(), (char *)p->second.c_str());
		}
	}

	WsXmlDocH doc;
	char *enumContext;
	WsXmlDocH enum_response = wsmc_action_enumerate(cl, (char *)resourceUri.c_str(),  options);

	try
	{
		if(ResourceNotFound(cl, enum_response))
		{
			wsmc_options_destroy(options);
			return;
		}
	}
	catch(GeneralWsmanException& e)
	{
		wsmc_options_destroy(options);
		throw e;
	}
	catch(exception& e)
	{
		wsmc_options_destroy(options);
		throw e;
	}
	
	enumContext = wsmc_get_enum_context(enum_response);
	ws_xml_destroy_doc(enum_response);

	while (enumContext != NULL && enumContext[0] != 0 ) {
		doc = wsmc_action_pull(cl, resourceUri.c_str(), options, enumContext);
		try
		{
			CheckWsmanResponse(cl, doc);
		}
		catch(exception& e)
		{
			wsmc_options_destroy(options);
			throw e;
		}
		enumRes.push_back(ExtractItems(doc));
		enumContext = wsmc_get_enum_context(doc);    
		ws_xml_destroy_doc(doc);
	}
	wsmc_options_destroy(options);
}

string OpenWsmanClient::Get(const string &resourceUri, const NameValuePairs *s) const
{
	client_opt_t *options = NULL;
	options = SetDefaultOptions();
	WsXmlDocH doc;
	// Add selectors.
	if (s) {
		for (PairsIterator p = s->begin(); p != s->end(); ++p) {
			if(p->second != "")
				wsmc_add_selector(options, 
						(char *)p->first.c_str(), (char *)p->second.c_str());
		}
	}
	doc = wsmc_action_get(cl, (char *)resourceUri.c_str(), options);
	wsmc_options_destroy(options);
	CheckWsmanResponse(cl, doc);
	string xml = ExtractPayload(doc);
	ws_xml_destroy_doc(doc);
	return xml;
}

string OpenWsmanClient::Put(const string &resourceUri, const string &content, const NameValuePairs *s) const
{
	client_opt_t *options = NULL;
	options = SetDefaultOptions();
	WsXmlDocH doc;
	// Add selectors.
	if (s) {
		for (PairsIterator p = s->begin(); p != s->end(); ++p) {
			if(p->second != "")
				wsmc_add_selector(options, 
						(char *)p->first.c_str(), (char *)p->second.c_str());
		}
	}
	doc = wsmc_action_put_fromtext(cl, resourceUri.c_str(), options, content.c_str(), content.length(), WSMAN_ENCODING);
	wsmc_options_destroy(options);
	CheckWsmanResponse(cl, doc);
	string xml = ExtractPayload(doc);
	ws_xml_destroy_doc(doc);
	return xml;
}

string OpenWsmanClient::Invoke(const string &resourceUri, const string &methodName, const string &content, const NameValuePairs *s) const
{
	client_opt_t *options = NULL;
	options = SetDefaultOptions();
	WsXmlDocH doc;
	string error;

	// Add selectors.
	if (s) {
		for (PairsIterator p = s->begin(); p != s->end(); ++p) {
			if(p->second != "")
				wsmc_add_selector(options, 
						(char *)p->first.c_str(), (char *)p->second.c_str());
		}
	}
	doc = wsmc_action_invoke_fromtext(cl, resourceUri.c_str(), options,
			(char *)methodName.c_str(), content.c_str(),
			content.length(), WSMAN_ENCODING);
	wsmc_options_destroy(options);
	CheckWsmanResponse(cl, doc);
	string xml = ExtractPayload(doc);
	ws_xml_destroy_doc(doc);
	return xml;
}

string ExtractPayload(WsXmlDocH& doc)
{
	WsXmlNodeH bodyNode = ws_xml_get_soap_body(doc);	  
	WsXmlNodeH payloadNode = ws_xml_get_child(bodyNode, 0, NULL, NULL);
	char *buf = NULL;
	wsmc_node_to_buf( payloadNode, &buf);
	string payload = string(buf);
	u_free(buf);
	return payload;
}

string ExtractItems(WsXmlDocH& doc)
{
	WsXmlNodeH bodyNode = ws_xml_get_soap_body(doc);
	WsXmlNodeH pullResponse = ws_xml_get_child(bodyNode, 0, XML_NS_ENUMERATION, WSENUM_PULL_RESP);
	WsXmlNodeH itemsNode = ws_xml_get_child(pullResponse, 0, XML_NS_ENUMERATION, WSENUM_ITEMS);
	WsXmlNodeH n = ws_xml_get_child(itemsNode, 0 , NULL, NULL );
	char *buf = NULL;
	wsmc_node_to_buf( n, &buf);
	string payload = string(buf);
	u_free(buf);
	return payload;
}

string XmlDocToString(WsXmlDocH& doc) {
	char *buf;
	int	  len;
	ws_xml_dump_memory_enc(doc, &buf, &len, WSMAN_ENCODING);
	string str = string(buf);	// This constructor copies the data.
	if (buf)
#ifdef _WIN32
		ws_xml_free_memory(buf);
#else
	u_free(buf);
#endif
	return str;
}

client_opt_t * SetDefaultOptions()
{
	client_opt_t *options = wsmc_options_init();
	return options;
}

bool CheckWsmanResponse(WsManClient* cl, WsXmlDocH& doc)
{
	long lastError = wsmc_get_last_error(cl);
	string error;
	if(lastError)
	{
		char tmp[10];
		error = "Failed to establish a connection with the server.\n";
		sprintf(tmp, "%ld", lastError);
		error.append("Openwsman last error = ").append(tmp);
		ws_xml_destroy_doc(doc);
		throw WsmanClientException(error.c_str(), WSMAN_CONNECT_ERROR);
	}
	long responseCode = wsmc_get_response_code(cl);
	if (responseCode != 200 &&
		responseCode != 400 &&
		responseCode != 500)
	{
		char tmp[10];
		error = "An HTTP error occurred.\n";
		sprintf(tmp, "%ld", responseCode);
		error.append("HTTP Error = ").append(tmp);
		ws_xml_destroy_doc(doc);
		throw WsmanClientException(error.c_str(), WSMAN_HTTP_ERROR);
	}
	if(!doc)
	{
		throw WsmanClientException("The Wsman response was NULL.");
	}
	if (wsmc_check_for_fault(doc)) {
		char tmp[10];
		WsManFault *fault = wsmc_fault_new();
		wsmc_get_fault_data(doc, fault);
		string subcode_s = fault->subcode ? string(fault->subcode) : "";	
		string code_s = fault->code ? string(fault->code) : "";
		string reason_s = fault->reason ? string(fault->reason) : "";
		string detail_s = fault->fault_detail ? string(fault->fault_detail) : "";
		ws_xml_destroy_doc(doc);
		wsmc_fault_destroy(fault);
		error = "A Soap Fault was received:";
		error.append("\nFaultCode: " + code_s);
		error.append("\nFaultSubCode: " + subcode_s);
		error.append("\nFaultReason: " + reason_s);
		error.append("\nFaultDetail: " + detail_s);
		sprintf(tmp, "%ld", responseCode);
		error.append("\nHttpCode:  = ").append(tmp);
		throw WsmanSoapFault(error.c_str(), code_s, subcode_s, reason_s, detail_s);
	}
	return true;
}

bool ResourceNotFound(WsManClient* cl, WsXmlDocH& enumerationRes)
{
	long responseCode = wsmc_get_response_code(cl);
	if(wsmc_get_last_error(cl) ||
		(responseCode != 200 && responseCode != 400 && responseCode != 500) ||
		!enumerationRes)
	{
		CheckWsmanResponse(cl, enumerationRes);
	}
	if (!wsmc_check_for_fault(enumerationRes))
	{
		return false;
	}
	WsManFault *fault = wsmc_fault_new();
	bool ret = false;
	wsmc_get_fault_data(enumerationRes, fault);
	string subcode_s = fault->subcode ? string(fault->subcode) : "";
	if(subcode_s.find("DestinationUnreachable") != string::npos)
	{
		ret = true;
	}
	wsmc_fault_destroy(fault);
	if(!ret)
	{
		CheckWsmanResponse(cl, enumerationRes);
	}
	return ret;
}

void OpenWsmanClient::SetAuth(const char *auth_method)
{
	wsman_transport_set_auth_method (cl , (char *)auth_method);
	if (wsmc_transport_get_auth_value(cl) == 0 ) {
		// Authentication method not supported, reverting to digest
		wsman_transport_set_auth_method(cl, "digest");
	}
}

#ifdef _WIN32
void OpenWsmanClient::SetClientCert(const char *oid, const char *cert, const bool local)
{
	if (cert) {
		wsman_transport_set_cainfo(cl, (char*)cert);
	}

	if (oid) {
		wsman_transport_set_caoid(cl, (char*)oid);
	}

	wsman_transport_set_calocal(cl, local);
}

#else
// Set server certificate params
// params: cainfo - string naming a file holding one or more certificates to verify the peer with.
//         capath - string naming a dierctory holding multiple CA certificates to verify the peer with.
// Give null arguments if you want curl to search for certificates inthe default path
// 
void OpenWsmanClient::SetServerCert(const char *cainfo, const char *capath)
{
  // This means curl verifies the server certificate
  wsman_transport_set_verify_peer(cl, 1);

  // This means the certificate must indicate that the server is the server to which you meant to connect.
  wsman_transport_set_verify_host(cl, 2);

  if (cainfo) {
    wsman_transport_set_cainfo(cl, (char*)cainfo);
  }
  if (capath) {
    wsman_transport_set_capath(cl, (char*)capath);
  }

}
      
// Set client certificates params
// params: cert - file name of your certificate.
//         key  - file name of your private key.
void OpenWsmanClient::SetClientCert(const char *cert, const char *key)
{
  if (cert) { 
    wsman_transport_set_cert(cl, (char*)cert);
  }
  if (key) {
    wsman_transport_set_key(cl, (char*)key);
  }
}
#endif
