/*
 * Copyright 2007-2018 The OpenSSL Project Authors. All Rights Reserved.
 * Copyright Nokia 2007-2018
 * Copyright Siemens AG 2015-2018
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 *
 * CRMF implementation by Martin Peylo, Miikka Viljanen, and David von Oheimb.
 */

#include <openssl/asn1t.h>

#include "crmf_int.h"

/* explicit #includes not strictly needed since implied by the above: */
#include <openssl/crmf.h>

ASN1_SEQUENCE(OSSL_CRMF_PRIVATEKEYINFO) = {
    ASN1_SIMPLE(OSSL_CRMF_PRIVATEKEYINFO, version, ASN1_INTEGER),
    ASN1_SIMPLE(OSSL_CRMF_PRIVATEKEYINFO, privateKeyAlgorithm, X509_ALGOR),
    ASN1_SIMPLE(OSSL_CRMF_PRIVATEKEYINFO, privateKey, ASN1_OCTET_STRING),
    ASN1_IMP_SET_OF_OPT(OSSL_CRMF_PRIVATEKEYINFO, attributes, X509_ATTRIBUTE, 0)
} ASN1_SEQUENCE_END(OSSL_CRMF_PRIVATEKEYINFO)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CRMF_PRIVATEKEYINFO)


ASN1_CHOICE(OSSL_CRMF_ENCKEYWITHID_IDENTIFIER) = {
    ASN1_SIMPLE(OSSL_CRMF_ENCKEYWITHID_IDENTIFIER, value.string, ASN1_UTF8STRING),
    ASN1_SIMPLE(OSSL_CRMF_ENCKEYWITHID_IDENTIFIER, value.generalName, GENERAL_NAME)
} ASN1_CHOICE_END(OSSL_CRMF_ENCKEYWITHID_IDENTIFIER)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CRMF_ENCKEYWITHID_IDENTIFIER)


ASN1_SEQUENCE(OSSL_CRMF_ENCKEYWITHID) = {
    ASN1_SIMPLE(OSSL_CRMF_ENCKEYWITHID, privateKey, OSSL_CRMF_PRIVATEKEYINFO),
    ASN1_OPT(OSSL_CRMF_ENCKEYWITHID, identifier,
             OSSL_CRMF_ENCKEYWITHID_IDENTIFIER)
} ASN1_SEQUENCE_END(OSSL_CRMF_ENCKEYWITHID)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CRMF_ENCKEYWITHID)


ASN1_SEQUENCE(OSSL_CRMF_CERTID) = {
    ASN1_SIMPLE(OSSL_CRMF_CERTID, issuer, GENERAL_NAME),
    ASN1_SIMPLE(OSSL_CRMF_CERTID, serialNumber, ASN1_INTEGER)
} ASN1_SEQUENCE_END(OSSL_CRMF_CERTID)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CRMF_CERTID)
IMPLEMENT_ASN1_DUP_FUNCTION(OSSL_CRMF_CERTID)


ASN1_SEQUENCE(OSSL_CRMF_ENCRYPTEDVALUE) = {
    ASN1_IMP_OPT(OSSL_CRMF_ENCRYPTEDVALUE, intendedAlg, X509_ALGOR, 0),
    ASN1_IMP_OPT(OSSL_CRMF_ENCRYPTEDVALUE, symmAlg, X509_ALGOR, 1),
    ASN1_IMP_OPT(OSSL_CRMF_ENCRYPTEDVALUE, encSymmKey, ASN1_BIT_STRING, 2),
    ASN1_IMP_OPT(OSSL_CRMF_ENCRYPTEDVALUE, keyAlg, X509_ALGOR, 3),
    ASN1_IMP_OPT(OSSL_CRMF_ENCRYPTEDVALUE, valueHint, ASN1_OCTET_STRING, 4),
    ASN1_SIMPLE(OSSL_CRMF_ENCRYPTEDVALUE, encValue, ASN1_BIT_STRING)
} ASN1_SEQUENCE_END(OSSL_CRMF_ENCRYPTEDVALUE)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CRMF_ENCRYPTEDVALUE)

ASN1_SEQUENCE(OSSL_CRMF_SINGLEPUBINFO) = {
    ASN1_SIMPLE(OSSL_CRMF_SINGLEPUBINFO, pubMethod, ASN1_INTEGER),
    ASN1_SIMPLE(OSSL_CRMF_SINGLEPUBINFO, pubLocation, GENERAL_NAME)
} ASN1_SEQUENCE_END(OSSL_CRMF_SINGLEPUBINFO)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CRMF_SINGLEPUBINFO)


ASN1_SEQUENCE(OSSL_CRMF_PKIPUBLICATIONINFO) = {
    ASN1_SIMPLE(OSSL_CRMF_PKIPUBLICATIONINFO, action, ASN1_INTEGER),
    ASN1_SEQUENCE_OF_OPT(OSSL_CRMF_PKIPUBLICATIONINFO, pubInfos,
                         OSSL_CRMF_SINGLEPUBINFO)
} ASN1_SEQUENCE_END(OSSL_CRMF_PKIPUBLICATIONINFO)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CRMF_PKIPUBLICATIONINFO)
IMPLEMENT_ASN1_DUP_FUNCTION(OSSL_CRMF_PKIPUBLICATIONINFO)


ASN1_SEQUENCE(OSSL_CRMF_PKMACVALUE) = {
    ASN1_SIMPLE(OSSL_CRMF_PKMACVALUE, algId, X509_ALGOR),
    ASN1_SIMPLE(OSSL_CRMF_PKMACVALUE, value, ASN1_BIT_STRING)
} ASN1_SEQUENCE_END(OSSL_CRMF_PKMACVALUE)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CRMF_PKMACVALUE)


ASN1_CHOICE(OSSL_CRMF_POPOPRIVKEY) = {
    ASN1_IMP(OSSL_CRMF_POPOPRIVKEY, value.thisMessage, ASN1_BIT_STRING, 0),
    ASN1_IMP(OSSL_CRMF_POPOPRIVKEY, value.subsequentMessage, ASN1_INTEGER, 1),
    ASN1_IMP(OSSL_CRMF_POPOPRIVKEY, value.dhMAC, ASN1_BIT_STRING, 2),
    ASN1_IMP(OSSL_CRMF_POPOPRIVKEY, value.agreeMAC, OSSL_CRMF_PKMACVALUE, 3),
    /*
     * TODO: This is not ASN1_NULL but CMS_ENVELOPEDDATA which should be somehow
     * taken from crypto/cms which exists now - this is not used anywhere so far
     */
    ASN1_IMP(OSSL_CRMF_POPOPRIVKEY, value.encryptedKey, ASN1_NULL, 4),
} ASN1_CHOICE_END(OSSL_CRMF_POPOPRIVKEY)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CRMF_POPOPRIVKEY)


ASN1_SEQUENCE(OSSL_CRMF_PBMPARAMETER) = {
    ASN1_SIMPLE(OSSL_CRMF_PBMPARAMETER, salt, ASN1_OCTET_STRING),
    ASN1_SIMPLE(OSSL_CRMF_PBMPARAMETER, owf, X509_ALGOR),
    ASN1_SIMPLE(OSSL_CRMF_PBMPARAMETER, iterationCount, ASN1_INTEGER),
    ASN1_SIMPLE(OSSL_CRMF_PBMPARAMETER, mac, X509_ALGOR)
} ASN1_SEQUENCE_END(OSSL_CRMF_PBMPARAMETER)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CRMF_PBMPARAMETER)


ASN1_CHOICE(OSSL_CRMF_POPOSIGNINGKEYINPUT_AUTHINFO) = {
    ASN1_EXP(OSSL_CRMF_POPOSIGNINGKEYINPUT_AUTHINFO, value.sender,
             GENERAL_NAME, 0),
    ASN1_SIMPLE(OSSL_CRMF_POPOSIGNINGKEYINPUT_AUTHINFO, value.publicKeyMAC,
                OSSL_CRMF_PKMACVALUE)
} ASN1_CHOICE_END(OSSL_CRMF_POPOSIGNINGKEYINPUT_AUTHINFO)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CRMF_POPOSIGNINGKEYINPUT_AUTHINFO)


ASN1_SEQUENCE(OSSL_CRMF_POPOSIGNINGKEYINPUT) = {
    ASN1_SIMPLE(OSSL_CRMF_POPOSIGNINGKEYINPUT, authInfo,
                OSSL_CRMF_POPOSIGNINGKEYINPUT_AUTHINFO),
    ASN1_SIMPLE(OSSL_CRMF_POPOSIGNINGKEYINPUT, publicKey, X509_PUBKEY)
} ASN1_SEQUENCE_END(OSSL_CRMF_POPOSIGNINGKEYINPUT)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CRMF_POPOSIGNINGKEYINPUT)


ASN1_SEQUENCE(OSSL_CRMF_POPOSIGNINGKEY) = {
    ASN1_IMP_OPT(OSSL_CRMF_POPOSIGNINGKEY, poposkInput,
                 OSSL_CRMF_POPOSIGNINGKEYINPUT, 0),
    ASN1_SIMPLE(OSSL_CRMF_POPOSIGNINGKEY, algorithmIdentifier, X509_ALGOR),
    ASN1_SIMPLE(OSSL_CRMF_POPOSIGNINGKEY, signature, ASN1_BIT_STRING)
} ASN1_SEQUENCE_END(OSSL_CRMF_POPOSIGNINGKEY)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CRMF_POPOSIGNINGKEY)


ASN1_CHOICE(OSSL_CRMF_POPO) = {
    ASN1_IMP(OSSL_CRMF_POPO, value.raVerified, ASN1_NULL, 0),
    ASN1_IMP(OSSL_CRMF_POPO, value.signature, OSSL_CRMF_POPOSIGNINGKEY, 1),
    ASN1_EXP(OSSL_CRMF_POPO, value.keyEncipherment, OSSL_CRMF_POPOPRIVKEY, 2),
    ASN1_EXP(OSSL_CRMF_POPO, value.keyAgreement, OSSL_CRMF_POPOPRIVKEY, 3)
} ASN1_CHOICE_END(OSSL_CRMF_POPO)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CRMF_POPO)


ASN1_ADB_TEMPLATE(attributetypeandvalue_default) = ASN1_OPT(
        OSSL_CRMF_ATTRIBUTETYPEANDVALUE, value.other, ASN1_ANY);
ASN1_ADB(OSSL_CRMF_ATTRIBUTETYPEANDVALUE) = {
    ADB_ENTRY(NID_id_regCtrl_regToken,
              ASN1_SIMPLE(OSSL_CRMF_ATTRIBUTETYPEANDVALUE,
                          value.regToken, ASN1_UTF8STRING)),
    ADB_ENTRY(NID_id_regCtrl_authenticator,
              ASN1_SIMPLE(OSSL_CRMF_ATTRIBUTETYPEANDVALUE,
                          value.authenticator, ASN1_UTF8STRING)),
    ADB_ENTRY(NID_id_regCtrl_pkiPublicationInfo,
              ASN1_SIMPLE(OSSL_CRMF_ATTRIBUTETYPEANDVALUE,
                          value.pkiPublicationInfo,
                          OSSL_CRMF_PKIPUBLICATIONINFO)),
    ADB_ENTRY(NID_id_regCtrl_oldCertID,
              ASN1_SIMPLE(OSSL_CRMF_ATTRIBUTETYPEANDVALUE,
                          value.oldCertID, OSSL_CRMF_CERTID)),
    ADB_ENTRY(NID_id_regCtrl_protocolEncrKey,
              ASN1_SIMPLE(OSSL_CRMF_ATTRIBUTETYPEANDVALUE,
                          value.protocolEncrKey, X509_PUBKEY)),
    ADB_ENTRY(NID_id_regInfo_utf8Pairs,
              ASN1_SIMPLE(OSSL_CRMF_ATTRIBUTETYPEANDVALUE,
                          value.utf8Pairs, ASN1_UTF8STRING)),
    ADB_ENTRY(NID_id_regInfo_certReq,
              ASN1_SIMPLE(OSSL_CRMF_ATTRIBUTETYPEANDVALUE,
                          value.certReq, OSSL_CRMF_CERTREQUEST)),
} ASN1_ADB_END(OSSL_CRMF_ATTRIBUTETYPEANDVALUE, 0, type, 0,
               &attributetypeandvalue_default_tt, NULL);


ASN1_SEQUENCE(OSSL_CRMF_ATTRIBUTETYPEANDVALUE) = {
    ASN1_SIMPLE(OSSL_CRMF_ATTRIBUTETYPEANDVALUE, type, ASN1_OBJECT),
    ASN1_ADB_OBJECT(OSSL_CRMF_ATTRIBUTETYPEANDVALUE)
} ASN1_SEQUENCE_END(OSSL_CRMF_ATTRIBUTETYPEANDVALUE)

IMPLEMENT_ASN1_FUNCTIONS(OSSL_CRMF_ATTRIBUTETYPEANDVALUE)
IMPLEMENT_ASN1_DUP_FUNCTION(OSSL_CRMF_ATTRIBUTETYPEANDVALUE)


ASN1_SEQUENCE(OSSL_CRMF_OPTIONALVALIDITY) = {
    ASN1_EXP_OPT(OSSL_CRMF_OPTIONALVALIDITY, notBefore, ASN1_TIME, 0),
    ASN1_EXP_OPT(OSSL_CRMF_OPTIONALVALIDITY, notAfter,  ASN1_TIME, 1)
} ASN1_SEQUENCE_END(OSSL_CRMF_OPTIONALVALIDITY)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CRMF_OPTIONALVALIDITY)


ASN1_SEQUENCE(OSSL_CRMF_CERTTEMPLATE) = {
    ASN1_IMP_OPT(OSSL_CRMF_CERTTEMPLATE, version,      ASN1_INTEGER, 0),
    /*
     * serialNumber MUST be omitted. This field is assigned by the CA
     * during certificate creation.
     */
    ASN1_IMP_OPT(OSSL_CRMF_CERTTEMPLATE, serialNumber, ASN1_INTEGER, 1),
    /*
     * signingAlg MUST be omitted. This field is assigned by the CA
     * during certificate creation.
     */
    ASN1_IMP_OPT(OSSL_CRMF_CERTTEMPLATE, signingAlg,   X509_ALGOR, 2),
    ASN1_EXP_OPT(OSSL_CRMF_CERTTEMPLATE, issuer,       X509_NAME, 3),
    ASN1_IMP_OPT(OSSL_CRMF_CERTTEMPLATE, validity,
                 OSSL_CRMF_OPTIONALVALIDITY, 4),
    ASN1_EXP_OPT(OSSL_CRMF_CERTTEMPLATE, subject,      X509_NAME, 5),
    ASN1_IMP_OPT(OSSL_CRMF_CERTTEMPLATE, publicKey,    X509_PUBKEY, 6),
    /* issuerUID is deprecated in version 2 */
    ASN1_IMP_OPT(OSSL_CRMF_CERTTEMPLATE, issuerUID,    ASN1_BIT_STRING, 7),
    /* subjectUID is deprecated in version 2 */
    ASN1_IMP_OPT(OSSL_CRMF_CERTTEMPLATE, subjectUID,   ASN1_BIT_STRING, 8),
    ASN1_IMP_SEQUENCE_OF_OPT(OSSL_CRMF_CERTTEMPLATE, extensions,
                             X509_EXTENSION, 9),
} ASN1_SEQUENCE_END(OSSL_CRMF_CERTTEMPLATE)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CRMF_CERTTEMPLATE)


ASN1_SEQUENCE(OSSL_CRMF_CERTREQUEST) = {
    ASN1_SIMPLE(OSSL_CRMF_CERTREQUEST, certReqId, ASN1_INTEGER),
    ASN1_SIMPLE(OSSL_CRMF_CERTREQUEST, certTemplate, OSSL_CRMF_CERTTEMPLATE),
    ASN1_SEQUENCE_OF_OPT(OSSL_CRMF_CERTREQUEST, controls,
                         OSSL_CRMF_ATTRIBUTETYPEANDVALUE)
} ASN1_SEQUENCE_END(OSSL_CRMF_CERTREQUEST)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CRMF_CERTREQUEST)
IMPLEMENT_ASN1_DUP_FUNCTION(OSSL_CRMF_CERTREQUEST)


ASN1_SEQUENCE(OSSL_CRMF_MSG) = {
    ASN1_SIMPLE(OSSL_CRMF_MSG, certReq, OSSL_CRMF_CERTREQUEST),
    ASN1_OPT(OSSL_CRMF_MSG, popo, OSSL_CRMF_POPO),
    ASN1_SEQUENCE_OF_OPT(OSSL_CRMF_MSG, regInfo,
                         OSSL_CRMF_ATTRIBUTETYPEANDVALUE)
} ASN1_SEQUENCE_END(OSSL_CRMF_MSG)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CRMF_MSG)


ASN1_ITEM_TEMPLATE(OSSL_CRMF_MSGS) =
    ASN1_EX_TEMPLATE_TYPE(ASN1_TFLG_SEQUENCE_OF, 0,
                          OSSL_CRMF_MSGS, OSSL_CRMF_MSG)
    ASN1_ITEM_TEMPLATE_END(OSSL_CRMF_MSGS)
IMPLEMENT_ASN1_FUNCTIONS(OSSL_CRMF_MSGS)

