/*
 * Generated by util/mkerr.pl DO NOT EDIT
 * Copyright 1995-2017 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <openssl/err.h>
#include <openssl/cmperr.h>

#ifndef OPENSSL_NO_ERR

static const ERR_STRING_DATA CMP_str_functs[] = {
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CALC_PROTECTION_PBMAC, 0),
     "CMP_calc_protection_pbmac"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CALC_PROTECTION_SIG, 0),
     "CMP_calc_protection_sig"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CERTCONF_NEW, 0), "CMP_certConf_new"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CERTORENCCERT_ENCCERT_GET1, 0),
     "CMP_CERTORENCCERT_encCert_get1"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CERTREPMESSAGE_CERTRESPONSE_GET0, 0),
     "CMP_CERTREPMESSAGE_certResponse_get0"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CERTREQ_NEW, 0), "CMP_certreq_new"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CERTRESPONSE_GET_CERTIFICATE, 0),
     "CMP_CERTRESPONSE_get_certificate"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CERTSTATUS_SET_CERTHASH, 0),
     "CMP_CERTSTATUS_set_certHash"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_CAPUBS_GET1, 0),
     "CMP_CTX_caPubs_get1"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_CAPUBS_NUM, 0), "CMP_CTX_caPubs_num"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_CAPUBS_POP, 0), "CMP_CTX_caPubs_pop"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_CREATE, 0), "CMP_CTX_create"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_EXTRACERTSIN_GET1, 0),
     "CMP_CTX_extraCertsIn_get1"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_EXTRACERTSIN_NUM, 0),
     "CMP_CTX_extraCertsIn_num"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_EXTRACERTSIN_POP, 0),
     "CMP_CTX_extraCertsIn_pop"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_EXTRACERTSOUT_NUM, 0),
     "CMP_CTX_extraCertsOut_num"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_EXTRACERTSOUT_PUSH1, 0),
     "CMP_CTX_extraCertsOut_push1"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_INIT, 0), "CMP_CTX_init"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_SET0_NEWPKEY, 0),
     "CMP_CTX_set0_newPkey"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_SET0_PKEY, 0), "CMP_CTX_set0_pkey"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_SET0_REQEXTENSIONS, 0),
     "CMP_CTX_set0_reqExtensions"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_SET0_TLSBIO, 0),
     "CMP_CTX_set0_tlsBIO"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_SET1_CAPUBS, 0),
     "CMP_CTX_set1_caPubs"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_SET1_CLCERT, 0),
     "CMP_CTX_set1_clCert"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_SET1_EXPECTED_SENDER, 0),
     "CMP_CTX_set1_expected_sender"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_SET1_EXTRACERTSIN, 0),
     "CMP_CTX_set1_extraCertsIn"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_SET1_EXTRACERTSOUT, 0),
     "CMP_CTX_set1_extraCertsOut"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_SET1_ISSUER, 0),
     "CMP_CTX_set1_issuer"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_SET1_NEWCLCERT, 0),
     "CMP_CTX_set1_newClCert"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_SET1_NEWPKEY, 0),
     "CMP_CTX_set1_newPkey"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_SET1_OLDCLCERT, 0),
     "CMP_CTX_set1_oldClCert"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_SET1_P10CSR, 0),
     "CMP_CTX_set1_p10CSR"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_SET1_PKEY, 0), "CMP_CTX_set1_pkey"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_SET1_PROXYNAME, 0),
     "CMP_CTX_set1_proxyName"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_SET1_RECIPIENT, 0),
     "CMP_CTX_set1_recipient"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_SET1_RECIPNONCE, 0),
     "CMP_CTX_set1_recipNonce"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_SET1_REFERENCEVALUE, 0),
     "CMP_CTX_set1_referenceValue"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_SET1_SECRETVALUE, 0),
     "CMP_CTX_set1_secretValue"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_SET1_SERVERNAME, 0),
     "CMP_CTX_set1_serverName"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_SET1_SERVERPATH, 0),
     "CMP_CTX_set1_serverPath"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_SET1_SRVCERT, 0),
     "CMP_CTX_set1_srvCert"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_SET1_SUBJECTNAME, 0),
     "CMP_CTX_set1_subjectName"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_SET1_TRANSACTIONID, 0),
     "CMP_CTX_set1_transactionID"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_SET_PROXYPORT, 0),
     "CMP_CTX_set_proxyPort"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_SET_SERVERPORT, 0),
     "CMP_CTX_set_serverPort"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_CTX_SUBJECTALTNAME_PUSH1, 0),
     "CMP_CTX_subjectAltName_push1"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_ERROR_NEW, 0), "CMP_error_new"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_EXEC_CR_SES, 0), "CMP_exec_CR_ses"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_EXEC_GENM_SES, 0), "CMP_exec_GENM_ses"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_EXEC_IR_SES, 0), "CMP_exec_IR_ses"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_EXEC_KUR_SES, 0), "CMP_exec_KUR_ses"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_EXEC_P10CR_SES, 0), "CMP_exec_P10CR_ses"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_EXEC_RR_SES, 0), "CMP_exec_RR_ses"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_GENM_NEW, 0), "CMP_genm_new"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_PKIHEADER_GENERALINFO_ITEM_PUSH0, 0),
     "CMP_PKIHEADER_generalInfo_item_push0"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_PKIMESSAGE_GENERALINFO_ITEMS_PUSH1, 0),
     "CMP_PKIMESSAGE_generalInfo_items_push1"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_PKIMESSAGE_GENM_ITEMS_PUSH1, 0),
     "CMP_PKIMESSAGE_genm_items_push1"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_PKIMESSAGE_GENM_ITEM_PUSH0, 0),
     "CMP_PKIMESSAGE_genm_item_push0"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_PKIMESSAGE_HTTP_PERFORM, 0),
     "CMP_PKIMESSAGE_http_perform"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_PKIMESSAGE_POLLRESPONSE_GET0, 0),
     "CMP_PKIMESSAGE_pollResponse_get0"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_PKIMESSAGE_PROTECT, 0),
     "CMP_PKIMESSAGE_protect"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_PKISTATUSINFO_PKISTATUS_GET, 0),
     "CMP_PKISTATUSINFO_PKIStatus_get"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_PKISTATUSINFO_PKISTATUS_GET_STRING, 0),
     "CMP_PKISTATUSINFO_PKIStatus_get_string"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_POLLREQ_NEW, 0), "CMP_pollReq_new"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_REVREPCONTENT_PKISTATUSINFO_GET, 0),
     "CMP_REVREPCONTENT_PKIStatusInfo_get"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_RR_NEW, 0), "CMP_rr_new"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_VALIDATE_CERT_PATH, 0),
     "CMP_validate_cert_path"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_VALIDATE_MSG, 0), "CMP_validate_msg"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_CMP_VERIFY_SIGNATURE, 0),
     "CMP_verify_signature"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_EXCHANGE_CERTCONF, 0), "exchange_certConf"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_EXCHANGE_ERROR, 0), "exchange_error"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_FIND_SRVCERT, 0), "find_srvcert"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_GET_CERT_STATUS, 0), "get_cert_status"},
    {ERR_PACK(ERR_LIB_CMP, CMP_F_POLLFORRESPONSE, 0), "pollForResponse"},
    {0, NULL}
};

static const ERR_STRING_DATA CMP_str_reasons[] = {
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_ALGORITHM_NOT_SUPPORTED),
    "algorithm not supported"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_CERTIFICATE_NOT_ACCEPTED),
    "certificate not accepted"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_CERTIFICATE_NOT_FOUND),
    "certificate not found"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_CERTRESPONSE_NOT_FOUND),
    "certresponse not found"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_CERT_AND_KEY_DO_NOT_MATCH),
    "cert and key do not match"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_CONNECT_TIMEOUT), "connect timeout"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_CP_NOT_RECEIVED), "cp not received"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_ENCOUNTERED_KEYUPDATEWARNING),
    "encountered keyupdatewarning"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_ENCOUNTERED_UNSUPPORTED_PKISTATUS),
    "encountered unsupported pkistatus"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_ENCOUNTERED_WAITING),
    "encountered waiting"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_ERROR_CALCULATING_PROTECTION),
    "error calculating protection"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_ERROR_CONNECTING), "error connecting"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_ERROR_CREATING_CERTCONF),
    "error creating certconf"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_ERROR_CREATING_CR), "error creating cr"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_ERROR_CREATING_ERROR),
    "error creating error"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_ERROR_CREATING_GENM),
    "error creating genm"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_ERROR_CREATING_IR), "error creating ir"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_ERROR_CREATING_KUR), "error creating kur"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_ERROR_CREATING_P10CR),
    "error creating p10cr"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_ERROR_CREATING_POLLREQ),
    "error creating pollreq"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_ERROR_CREATING_RR), "error creating rr"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_ERROR_DECODING_CERTIFICATE),
    "error decoding certificate"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_ERROR_DECODING_MESSAGE),
    "error decoding message"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_ERROR_DECRYPTING_CERTIFICATE),
    "error decrypting certificate"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_ERROR_DECRYPTING_ENCCERT),
    "error decrypting enccert"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_ERROR_DECRYPTING_KEY),
    "error decrypting key"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_ERROR_DECRYPTING_SYMMETRIC_KEY),
    "error decrypting symmetric key"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_ERROR_LEARNING_TRANSACTIONID),
    "error learning transactionid"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_ERROR_NONCES_DO_NOT_MATCH),
    "error nonces do not match"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_ERROR_PARSING_PKISTATUS),
    "error parsing pkistatus"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_ERROR_PKISTATUSINFO_NOT_FOUND),
    "error pkistatusinfo not found"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_ERROR_PROTECTING_MESSAGE),
    "error protecting message"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_ERROR_PUSHING_GENERALINFO_ITEM),
    "error pushing generalinfo item"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_ERROR_PUSHING_GENERALINFO_ITEMS),
    "error pushing generalinfo items"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_ERROR_PUSHING_GENM_ITEMS),
    "error pushing genm items"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_ERROR_SENDING_REQUEST),
    "error sending request"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_ERROR_SETTING_CERTHASH),
    "error setting certhash"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_ERROR_STATUS_NOT_FOUND),
    "error status not found"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_ERROR_TRANSACTIONID_UNMATCHED),
    "error transactionid unmatched"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_ERROR_VALIDATING_PROTECTION),
    "error validating protection"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_FAILED_TO_RECEIVE_PKIMESSAGE),
    "failed to receive pkimessage"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_FAILED_TO_SEND_REQUEST),
    "failed to send request"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_GENP_NOT_RECEIVED), "genp not received"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_INVALID_ARGS), "invalid args"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_INVALID_CONTEXT), "invalid context"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_INVALID_KEY), "invalid key"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_INVALID_PARAMETERS), "invalid parameters"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_IP_NOT_RECEIVED), "ip not received"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_KUP_NOT_RECEIVED), "kup not received"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_MISSING_KEY_INPUT_FOR_CREATING_PROTECTION),
    "missing key input for creating protection"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_NO_SECRET_VALUE_GIVEN_FOR_PBMAC),
    "no secret value given for pbmac"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_NO_TRUSTED_CERTIFICATES_SET),
    "no trusted certificates set"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_NO_VALID_SERVER_CERT_FOUND),
    "no valid server cert found"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_NO_VALID_SRVCERT_FOUND),
    "no valid srvcert found"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_NULL_ARGUMENT), "null argument"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_OUT_OF_MEMORY), "out of memory"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_PKIBODY_ERROR), "pkibody error"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_PKICONF_NOT_RECEIVED),
    "pkiconf not received"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_POLLREP_NOT_RECEIVED),
    "pollrep not received"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_READ_TIMEOUT), "read timeout"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_RECEIVED_NEGATIVE_CHECKAFTER_IN_POLLREP),
    "received negative checkafter in pollrep"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_REQUEST_REJECTED_BY_CA),
    "request rejected by ca"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_RP_NOT_RECEIVED), "rp not received"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_SENDER_GENERALNAME_TYPE_NOT_SUPPORTED),
    "sender generalname type not supported"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_SERVER_NOT_REACHABLE),
    "server not reachable"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_TLS_ERROR), "tls error"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_UNABLE_TO_CREATE_CONTEXT),
    "unable to create context"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_UNEXPECTED_PKIBODY), "unexpected pkibody"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_UNEXPECTED_PKISTATUS),
    "unexpected pkistatus"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_UNEXPECTED_SENDER), "unexpected sender"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_UNKNOWN_ALGORITHM_ID),
    "unknown algorithm id"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_UNKNOWN_CERTTYPE), "unknown certtype"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_UNKNOWN_CERT_TYPE), "unknown cert type"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_UNKNOWN_PKISTATUS), "unknown pkistatus"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_UNSUPPORTED_ALGORITHM),
    "unsupported algorithm"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_UNSUPPORTED_CIPHER), "unsupported cipher"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_UNSUPPORTED_KEY_TYPE),
    "unsupported key type"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_UNSUPPORTED_PROTECTION_ALG_DHBASEDMAC),
    "unsupported protection alg dhbasedmac"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_WRONG_ALGORITHM_OID),
    "wrong algorithm oid"},
    {ERR_PACK(ERR_LIB_CMP, 0, CMP_R_WRONG_KEY_USAGE), "wrong key usage"},
    {0, NULL}
};

#endif

int ERR_load_CMP_strings(void)
{
#ifndef OPENSSL_NO_ERR
    if (ERR_func_error_string(CMP_str_functs[0].error) == NULL) {
        ERR_load_strings_const(CMP_str_functs);
        ERR_load_strings_const(CMP_str_reasons);
    }
#endif
    return 1;
}
