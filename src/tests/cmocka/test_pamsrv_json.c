/*
    SSSD

    Unit test for pamsrv_json

    Authors:
        Iker Pedrosa <ipedrosa@redhat.com>

    Copyright (C) 2024 Red Hat

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <jansson.h>
#include <popt.h>

#include "tests/cmocka/common_mock.h"

#include "src/responder/pam/pamsrv.h"
#include "src/responder/pam/pamsrv_json.h"

#define PASSWORD_PROMPT     "Password"
#define OAUTH2_INIT_PROMPT  "Log In"
#define OAUTH2_LINK_PROMPT  "Log in online with another device"
#define OAUTH2_URI          "short.url.com/tmp\0"
#define OAUTH2_URI_COMP     "\0"
#define OAUTH2_CODE         "1234-5678"
#define OAUTH2_STR          OAUTH2_URI OAUTH2_URI_COMP OAUTH2_CODE
#define SC_INIT_PROMPT      "Insert smartcard"
#define SC_PIN_PROMPT       "Smartcard PIN"
#define PASSKEY_INIT_PROMPT "Insert security key"
#define PASSKEY_PIN_PROMPT  "Security key PIN"
#define PASSKEY_TOUCH_PROMPT "Touch security key"

#define SC1_CERT_USER       "cert_user1\0"
#define SC1_TOKEN_NAME      "token_name1\0"
#define SC1_MODULE_NAME     "module_name1\0"
#define SC1_KEY_ID          "key_id1\0"
#define SC1_LABEL           "label1\0"
#define SC1_PROMPT_STR      "prompt1\0"
#define SC1_PAM_CERT_USER   "pam_cert_user1"
#define SC1_STR             SC1_CERT_USER SC1_TOKEN_NAME SC1_MODULE_NAME SC1_KEY_ID \
                            SC1_LABEL SC1_PROMPT_STR SC1_PAM_CERT_USER
#define SC2_CERT_USER       "cert_user2\0"
#define SC2_TOKEN_NAME      "token_name2\0"
#define SC2_MODULE_NAME     "module_name2\0"
#define SC2_KEY_ID          "key_id2\0"
#define SC2_LABEL           "label2\0"
#define SC2_PROMPT_STR      "prompt2\0"
#define SC2_PAM_CERT_USER   "pam_cert_user2"
#define SC2_STR             SC2_CERT_USER SC2_TOKEN_NAME SC2_MODULE_NAME SC2_KEY_ID \
                            SC2_LABEL SC2_PROMPT_STR SC2_PAM_CERT_USER

#define BASIC_PASSWORD              "\"password\": {" \
                                    "\"name\": \"Password\", \"role\": \"password\", " \
                                    "\"selectable\": true, \"prompt\": \"Password\"}"
#define BASIC_OAUTH2                "\"eidp\": {" \
                                    "\"name\": \"Web Login\", \"role\": \"eidp\", " \
                                    "\"selectable\": true, " \
                                    "\"init_prompt\": \"" OAUTH2_INIT_PROMPT "\", " \
                                    "\"link_prompt\": \"" OAUTH2_LINK_PROMPT "\", " \
                                    "\"uri\": \"short.url.com/tmp\", \"code\": \"1234-5678\", " \
                                    "\"timeout\": 300}"
#define BASIC_SC                    "\"smartcard:1\": {" \
                                    "\"name\": \"prompt1\", \"role\": \"smartcard\", " \
                                    "\"selectable\": true, " \
                                    "\"init_instruction\": \"" SC_INIT_PROMPT "\", " \
                                    "\"pin_prompt\": \"" SC_PIN_PROMPT "\"}"
#define MULTIPLE_SC                 "\"smartcard:1\": {" \
                                    "\"name\": \"prompt1\", \"role\": \"smartcard\", " \
                                    "\"selectable\": true, " \
                                    "\"init_instruction\": \"" SC_INIT_PROMPT "\", " \
                                    "\"pin_prompt\": \"" SC_PIN_PROMPT "\"}, " \
                                    "\"smartcard:2\": {" \
                                    "\"name\": \"prompt2\", \"role\": \"smartcard\", " \
                                    "\"selectable\": true, " \
                                    "\"init_instruction\": \"" SC_INIT_PROMPT "\", " \
                                    "\"pin_prompt\": \"" SC_PIN_PROMPT "\"}"
#define BASIC_PASSKEY               "\"passkey\": {" \
                                    "\"name\": \"passkey\", \"role\": \"passkey\", " \
                                    "\"selectable\": true, " \
                                    "\"init_instruction\": \"" PASSKEY_INIT_PROMPT "\", " \
                                    "\"pin_request\": true, \"pin_attempts\": 8, " \
                                    "\"pin_prompt\": \"" PASSKEY_PIN_PROMPT "\", " \
                                    "\"touch_instruction\": \"" PASSKEY_TOUCH_PROMPT "\"}"
#define MECHANISMS_PASSWORD         "{" BASIC_PASSWORD "}"
#define MECHANISMS_OAUTH2           "{" BASIC_OAUTH2 "}"
#define MECHANISMS_SC1              "{" BASIC_SC "}"
#define MECHANISMS_SC2              "{" MULTIPLE_SC "}"
#define MECHANISMS_PASSKEY          "{" BASIC_PASSKEY "}"
#define PRIORITY_ALL                "[\"passkey\", \"eidp\", \"smartcard:1\", " \
                                    "\"smartcard:2\", \"password\"]"
#define AUTH_SELECTION_PASSWORD     "{\"auth-selection\": {\"mechanisms\": " \
                                    MECHANISMS_PASSWORD ", " \
                                    "\"priority\": [\"password\"]}}"
#define AUTH_SELECTION_OAUTH2       "{\"auth-selection\": {\"mechanisms\": " \
                                    MECHANISMS_OAUTH2 ", " \
                                    "\"priority\": [\"eidp\"]}}"
#define AUTH_SELECTION_SC           "{\"auth-selection\": {\"mechanisms\": " \
                                    MECHANISMS_SC2 ", " \
                                    "\"priority\": [\"smartcard:1\", \"smartcard:2\"]}}"
#define AUTH_SELECTION_PASSKEY      "{\"auth-selection\": {\"mechanisms\": " \
                                    MECHANISMS_PASSKEY ", " \
                                    "\"priority\": [\"passkey\"]}}"
#define AUTH_SELECTION_ALL          "{\"auth-selection\": {\"mechanisms\": {" \
                                    BASIC_PASSWORD ", " \
                                    BASIC_OAUTH2 ", " \
                                    MULTIPLE_SC ", " \
                                    BASIC_PASSKEY "}, " \
                                    "\"priority\": " PRIORITY_ALL "}}"

#define PASSWORD_CONTENT            "{\"password\": \"ThePassword\"}"
#define PIN_CONTENT                 "{\"pin\": \"ThePIN\"}"
#define AUTH_MECH_REPLY_PASSWORD    "{\"auth-selection\": {" \
                                    "\"status\": \"Ok\", \"password\": " \
                                    PASSWORD_CONTENT "}}"
#define AUTH_MECH_REPLY_OAUTH2      "{\"auth-selection\": {" \
                                    "\"status\": \"Ok\", \"eidp\": {}}}"
#define AUTH_MECH_REPLY_SMARTCARD   "{\"auth-selection\": {" \
                                    "\"status\": \"Ok\", \"smartcard:1\": " \
                                    PIN_CONTENT "}}"
#define AUTH_MECH_REPLY_PASSKEY     "{\"auth-selection\": {" \
                                    "\"status\": \"Ok\", \"passkey\": " \
                                    PIN_CONTENT "}}"
#define AUTH_MECH_ERRONEOUS         "{\"auth-selection\": {" \
                                    "\"status\": \"Ok\", \"lololo\": {}}}"

struct cert_auth_info {
    char *cert_user;
    char *cert;
    char *token_name;
    char *module_name;
    char *key_id;
    char *label;
    char *prompt_str;
    char *pam_cert_user;
    char *choice_list_id;
    struct cert_auth_info *prev;
    struct cert_auth_info *next;
};

/***********************
 * SETUP AND TEARDOWN
 **********************/
static int setup(void **state)
{
    struct auth_data *auth_data = NULL;

    assert_true(leak_check_setup());

    auth_data = talloc_zero(global_talloc_context, struct auth_data);
    assert_non_null(auth_data);
    auth_data->pswd = talloc_zero(auth_data, struct password_data);
    assert_non_null(auth_data->pswd);
    auth_data->oauth2 = talloc_zero(auth_data, struct oauth2_data);
    assert_non_null(auth_data->oauth2);
    auth_data->sc = talloc_zero(auth_data, struct sc_data);
    assert_non_null(auth_data->sc);
    auth_data->sc->names = talloc_array(auth_data->sc, char *, 3);
    assert_non_null(auth_data->sc->names);
    auth_data->passkey = talloc_zero(auth_data, struct passkey_data);
    assert_non_null(auth_data->passkey);

    auth_data->pswd->enabled = false;
    auth_data->oauth2->enabled = false;
    auth_data->sc->enabled = false;
    auth_data->passkey->enabled = false;

    check_leaks_push(auth_data);
    *state = (void *)auth_data;
    return 0;
}

static int teardown(void **state)
{
    struct auth_data *auth_data = talloc_get_type_abort(*state, struct auth_data);

    assert_non_null(auth_data);
    assert_true(check_leaks_pop(auth_data));
    talloc_free(auth_data);
    assert_true(leak_check_teardown());

    return 0;
}

/***********************
 * WRAPPERS
 **********************/
int __real_json_array_append_new(json_t *array, json_t *value);

int
__wrap_json_array_append_new(json_t *array, json_t *value)
{
    int fail;
    int ret;

    fail = mock();

    if(fail) {
        ret = -1;
    } else {
        ret = __real_json_array_append_new(array, value);
    }

    return ret;
}

/***********************
 * TEST
 **********************/
void test_get_cert_list(void **state)
{
    TALLOC_CTX *test_ctx = NULL;
    struct cert_auth_info *cert_list = NULL;
    struct cert_auth_info *item = NULL;
    struct pam_data *pd = NULL;
    const char *expected_token_name[] = {SC1_TOKEN_NAME, SC2_TOKEN_NAME};
    const char *expected_module_name[] = {SC1_MODULE_NAME, SC2_MODULE_NAME};
    const char *expected_key_id[] = {SC1_KEY_ID, SC2_KEY_ID};
    const char *expected_label[] = {SC1_LABEL, SC2_LABEL};
    const char *expected_prompt_str[] = {SC1_PROMPT_STR, SC2_PROMPT_STR};
    int i = 0;
    int len;
    int ret;

    test_ctx = talloc_new(NULL);
    assert_non_null(test_ctx);
    pd = talloc_zero(test_ctx, struct pam_data);
    assert_non_null(pd);

    len = strlen(SC1_CERT_USER)+1+strlen(SC1_TOKEN_NAME)+1+
          strlen(SC1_MODULE_NAME)+1+strlen(SC1_KEY_ID)+1+strlen(SC1_LABEL)+1+
          strlen(SC1_PROMPT_STR)+1+strlen(SC1_PAM_CERT_USER)+1;
    ret = pam_add_response(pd, SSS_PAM_CERT_INFO, len, discard_const(SC1_STR));
    assert_int_equal(ret, EOK);
    len = strlen(SC2_CERT_USER)+1+strlen(SC2_TOKEN_NAME)+1+
          strlen(SC2_MODULE_NAME)+1+strlen(SC2_KEY_ID)+1+strlen(SC2_LABEL)+1+
          strlen(SC2_PROMPT_STR)+1+strlen(SC2_PAM_CERT_USER)+1;
    ret = pam_add_response(pd, SSS_PAM_CERT_INFO, len, discard_const(SC2_STR));
    assert_int_equal(ret, EOK);

    ret = get_cert_list(test_ctx, pd, &cert_list);
    assert_int_equal(ret, EOK);
    DLIST_FOR_EACH(item, cert_list) {
        assert_string_equal(expected_token_name[i], item->token_name);
        assert_string_equal(expected_module_name[i], item->module_name);
        assert_string_equal(expected_key_id[i], item->key_id);
        assert_string_equal(expected_label[i], item->label);
        assert_string_equal(expected_prompt_str[i], item->prompt_str);
        i++;
    }

    talloc_free(test_ctx);
}

void test_get_cert_names(void **state)
{
    TALLOC_CTX *test_ctx = NULL;
    struct auth_data *auth_data = talloc_get_type_abort(*state, struct auth_data);
    struct cert_auth_info *cert_list = NULL;
    struct cert_auth_info *cai = NULL;
    int ret;

    test_ctx = talloc_new(NULL);
    assert_non_null(test_ctx);
    cai = talloc_zero(test_ctx, struct cert_auth_info);
    assert_non_null(cai);
    cai->prompt_str = discard_const(SC1_PROMPT_STR);
    DLIST_ADD(cert_list, cai);
    cai = talloc_zero(test_ctx, struct cert_auth_info);
    assert_non_null(cai);
    cai->prompt_str = discard_const(SC2_PROMPT_STR);
    DLIST_ADD(cert_list, cai);

    ret = get_cert_names(test_ctx, cert_list, auth_data);
    assert_int_equal(ret, EOK);
    assert_string_equal(auth_data->sc->names[0], SC2_PROMPT_STR);
    assert_string_equal(auth_data->sc->names[1], SC1_PROMPT_STR);

    talloc_free(test_ctx);
}

void test_json_format_mechanisms_password(void **state)
{
    struct auth_data *auth_data = talloc_get_type_abort(*state, struct auth_data);
    json_t *mechs = NULL;
    char *string;
    int ret;

    auth_data->pswd->enabled = true;
    auth_data->pswd->prompt = discard_const(PASSWORD_PROMPT);

    ret = json_format_mechanisms(auth_data, &mechs);
    assert_int_equal(ret, EOK);

    string = json_dumps(mechs, 0);
    assert_string_equal(string, MECHANISMS_PASSWORD);

    json_decref(mechs);
    free(string);
}

void test_json_format_mechanisms_oauth2(void **state)
{
    struct auth_data *auth_data = talloc_get_type_abort(*state, struct auth_data);
    json_t *mechs = NULL;
    char *string;
    int ret;

    auth_data->oauth2->enabled = true;
    auth_data->oauth2->uri = discard_const(OAUTH2_URI);
    auth_data->oauth2->code = discard_const(OAUTH2_CODE);
    auth_data->oauth2->init_prompt = discard_const(OAUTH2_INIT_PROMPT);
    auth_data->oauth2->link_prompt = discard_const(OAUTH2_LINK_PROMPT);

    ret = json_format_mechanisms(auth_data, &mechs);
    assert_int_equal(ret, EOK);

    string = json_dumps(mechs, 0);
    assert_string_equal(string, MECHANISMS_OAUTH2);

    json_decref(mechs);
    free(string);
}

void test_json_format_mechanisms_sc1(void **state)
{
    struct auth_data *auth_data = talloc_get_type_abort(*state, struct auth_data);
    json_t *mechs = NULL;
    char *string;
    int ret;

    auth_data->sc->enabled = true;
    auth_data->sc->names[0] = talloc_strdup(auth_data->sc->names, SC1_PROMPT_STR);
    assert_non_null(auth_data->sc->names[0]);
    auth_data->sc->names[1] = NULL;
    auth_data->sc->init_prompt = discard_const(SC_INIT_PROMPT);
    auth_data->sc->pin_prompt = discard_const(SC_PIN_PROMPT);

    ret = json_format_mechanisms(auth_data, &mechs);
    assert_int_equal(ret, EOK);

    string = json_dumps(mechs, 0);
    assert_string_equal(string, MECHANISMS_SC1);

    json_decref(mechs);
    free(string);
    talloc_free(auth_data->sc->names[0]);
}

void test_json_format_mechanisms_sc2(void **state)
{
    struct auth_data *auth_data = talloc_get_type_abort(*state, struct auth_data);
    json_t *mechs = NULL;
    char *string;
    int ret;

    auth_data->sc->enabled = true;
    auth_data->sc->names[0] = talloc_strdup(auth_data->sc->names, SC1_PROMPT_STR);
    assert_non_null(auth_data->sc->names[0]);
    auth_data->sc->names[1] = talloc_strdup(auth_data->sc->names, SC2_PROMPT_STR);
    assert_non_null(auth_data->sc->names[1]);
    auth_data->sc->names[2] = NULL;
    auth_data->sc->init_prompt = discard_const(SC_INIT_PROMPT);
    auth_data->sc->pin_prompt = discard_const(SC_PIN_PROMPT);

    ret = json_format_mechanisms(auth_data, &mechs);
    assert_int_equal(ret, EOK);

    string = json_dumps(mechs, 0);
    assert_string_equal(string, MECHANISMS_SC2);

    json_decref(mechs);
    free(string);
    talloc_free(auth_data->sc->names[0]);
    talloc_free(auth_data->sc->names[1]);
}

void test_json_format_mechanisms_passkey(void **state)
{
    struct auth_data *auth_data = talloc_get_type_abort(*state, struct auth_data);
    json_t *mechs = NULL;
    char *string;
    int ret;

    auth_data->passkey->enabled = true;
    auth_data->passkey->init_prompt = discard_const(PASSKEY_INIT_PROMPT);
    auth_data->passkey->pin_request = true;
    auth_data->passkey->pin_attempts = 8;
    auth_data->passkey->pin_prompt = discard_const(PASSKEY_PIN_PROMPT);
    auth_data->passkey->touch_prompt = discard_const(PASSKEY_TOUCH_PROMPT);

    ret = json_format_mechanisms(auth_data, &mechs);
    assert_int_equal(ret, EOK);

    string = json_dumps(mechs, 0);
    assert_string_equal(string, MECHANISMS_PASSKEY);

    json_decref(mechs);
    free(string);
}

void test_json_format_priority_all(void **state)
{
    struct auth_data *auth_data = talloc_get_type_abort(*state, struct auth_data);
    json_t *priority = NULL;
    char *string;
    int ret;

    auth_data->pswd->enabled = true;
    auth_data->oauth2->enabled = true;
    auth_data->sc->enabled = true;
    auth_data->sc->names[0] = talloc_strdup(auth_data->sc->names, SC1_LABEL);
    assert_non_null(auth_data->sc->names[0]);
    auth_data->sc->names[1] = talloc_strdup(auth_data->sc->names, SC2_LABEL);
    assert_non_null(auth_data->sc->names[1]);
    auth_data->sc->names[2] = NULL;
    auth_data->passkey->enabled = true;

     will_return_count(__wrap_json_array_append_new, false, 5);
    ret = json_format_priority(auth_data, &priority);
    assert_int_equal(ret, EOK);

    string = json_dumps(priority, 0);
    assert_string_equal(string, PRIORITY_ALL);

    json_decref(priority);
    free(string);
    talloc_free(auth_data->sc->names[0]);
    talloc_free(auth_data->sc->names[1]);
}

void test_json_format_auth_selection_password(void **state)
{
    TALLOC_CTX *test_ctx = NULL;
    struct auth_data *auth_data = talloc_get_type_abort(*state, struct auth_data);
    char *json_msg = NULL;
    int ret;

    test_ctx = talloc_new(NULL);
    assert_non_null(test_ctx);
    auth_data->pswd->enabled = true;
    auth_data->pswd->prompt = discard_const(PASSWORD_PROMPT);

    will_return(__wrap_json_array_append_new, false);
    ret = json_format_auth_selection(test_ctx, auth_data, &json_msg);
    assert_int_equal(ret, EOK);
    assert_string_equal(json_msg, AUTH_SELECTION_PASSWORD);

    talloc_free(test_ctx);
}

void test_json_format_auth_selection_oauth2(void **state)
{
    TALLOC_CTX *test_ctx = NULL;
    struct auth_data *auth_data = talloc_get_type_abort(*state, struct auth_data);
    char *json_msg = NULL;
    int ret;

    test_ctx = talloc_new(NULL);
    assert_non_null(test_ctx);
    auth_data->oauth2->enabled = true;
    auth_data->oauth2->uri = discard_const(OAUTH2_URI);
    auth_data->oauth2->code = discard_const(OAUTH2_CODE);
    auth_data->oauth2->init_prompt = discard_const(OAUTH2_INIT_PROMPT);
    auth_data->oauth2->link_prompt = discard_const(OAUTH2_LINK_PROMPT);

    will_return(__wrap_json_array_append_new, false);
    ret = json_format_auth_selection(test_ctx, auth_data, &json_msg);
    assert_int_equal(ret, EOK);
    assert_string_equal(json_msg, AUTH_SELECTION_OAUTH2);

    talloc_free(test_ctx);
}

void test_json_format_auth_selection_sc(void **state)
{
    TALLOC_CTX *test_ctx = NULL;
    struct auth_data *auth_data = talloc_get_type_abort(*state, struct auth_data);
    char *json_msg = NULL;
    int ret;

    test_ctx = talloc_new(NULL);
    assert_non_null(test_ctx);
    auth_data->sc->enabled = true;
    auth_data->sc->names[0] = talloc_strdup(auth_data->sc->names, SC1_PROMPT_STR);
    assert_non_null(auth_data->sc->names[0]);
    auth_data->sc->names[1] = talloc_strdup(auth_data->sc->names, SC2_PROMPT_STR);
    assert_non_null(auth_data->sc->names[1]);
    auth_data->sc->names[2] = NULL;
    auth_data->sc->init_prompt = discard_const(SC_INIT_PROMPT);
    auth_data->sc->pin_prompt = discard_const(SC_PIN_PROMPT);

    will_return(__wrap_json_array_append_new, false);
    will_return(__wrap_json_array_append_new, false);
    ret = json_format_auth_selection(test_ctx, auth_data, &json_msg);
    assert_int_equal(ret, EOK);
    assert_string_equal(json_msg, AUTH_SELECTION_SC);

    talloc_free(auth_data->sc->names[0]);
    talloc_free(auth_data->sc->names[1]);
    talloc_free(test_ctx);
}

void test_json_format_auth_selection_passkey(void **state)
{
    TALLOC_CTX *test_ctx = NULL;
    struct auth_data *auth_data = talloc_get_type_abort(*state, struct auth_data);
    char *json_msg = NULL;
    int ret;

    test_ctx = talloc_new(NULL);
    assert_non_null(test_ctx);
    auth_data->passkey->enabled = true;
    auth_data->passkey->init_prompt = discard_const(PASSKEY_INIT_PROMPT);
    auth_data->passkey->pin_request = true;
    auth_data->passkey->pin_attempts = 8;
    auth_data->passkey->pin_prompt = discard_const(PASSKEY_PIN_PROMPT);
    auth_data->passkey->touch_prompt = discard_const(PASSKEY_TOUCH_PROMPT);

    will_return(__wrap_json_array_append_new, false);
    ret = json_format_auth_selection(test_ctx, auth_data, &json_msg);
    assert_int_equal(ret, EOK);
    assert_string_equal(json_msg, AUTH_SELECTION_PASSKEY);

    talloc_free(test_ctx);
}

void test_json_format_auth_selection_all(void **state)
{
    TALLOC_CTX *test_ctx = NULL;
    struct auth_data *auth_data = talloc_get_type_abort(*state, struct auth_data);
    char *json_msg = NULL;
    int ret;

    test_ctx = talloc_new(NULL);
    assert_non_null(test_ctx);
    auth_data->pswd->enabled = true;
    auth_data->pswd->prompt = discard_const(PASSWORD_PROMPT);
    auth_data->oauth2->enabled = true;
    auth_data->oauth2->uri = discard_const(OAUTH2_URI);
    auth_data->oauth2->code = discard_const(OAUTH2_CODE);
    auth_data->oauth2->init_prompt = discard_const(OAUTH2_INIT_PROMPT);
    auth_data->oauth2->link_prompt = discard_const(OAUTH2_LINK_PROMPT);
    auth_data->sc->enabled = true;
    auth_data->sc->names = talloc_array(test_ctx, char *, 3);
    assert_non_null(auth_data->sc->names);
    auth_data->sc->names[0] = talloc_strdup(auth_data->sc->names, SC1_PROMPT_STR);
    assert_non_null(auth_data->sc->names[0]);
    auth_data->sc->names[1] = talloc_strdup(auth_data->sc->names, SC2_PROMPT_STR);
    assert_non_null(auth_data->sc->names[1]);
    auth_data->sc->names[2] = NULL;
    auth_data->sc->init_prompt = discard_const(SC_INIT_PROMPT);
    auth_data->sc->pin_prompt = discard_const(SC_PIN_PROMPT);
    auth_data->passkey->enabled = true;
    auth_data->passkey->init_prompt = discard_const(PASSKEY_INIT_PROMPT);
    auth_data->passkey->pin_request = true;
    auth_data->passkey->pin_attempts = 8;
    auth_data->passkey->pin_prompt = discard_const(PASSKEY_PIN_PROMPT);
    auth_data->passkey->touch_prompt = discard_const(PASSKEY_TOUCH_PROMPT);

     will_return_count(__wrap_json_array_append_new, false, 5);
    ret = json_format_auth_selection(test_ctx, auth_data, &json_msg);
    assert_int_equal(ret, EOK);
    assert_string_equal(json_msg, AUTH_SELECTION_ALL);

    talloc_free(auth_data->sc->names[0]);
    talloc_free(auth_data->sc->names[1]);
    talloc_free(test_ctx);
}

void test_json_format_auth_selection_failure(void **state)
{
    TALLOC_CTX *test_ctx = NULL;
    struct auth_data *auth_data = talloc_get_type_abort(*state, struct auth_data);
    char *json_msg = NULL;
    int ret;

    test_ctx = talloc_new(NULL);
    assert_non_null(test_ctx);
    auth_data->pswd->enabled = true;
    auth_data->pswd->prompt = discard_const(PASSWORD_PROMPT);
    auth_data->oauth2->enabled = true;
    auth_data->oauth2->uri = discard_const(OAUTH2_URI);
    auth_data->oauth2->code = discard_const(OAUTH2_CODE);
    auth_data->oauth2->init_prompt = discard_const(OAUTH2_INIT_PROMPT);
    auth_data->oauth2->link_prompt = discard_const(OAUTH2_LINK_PROMPT);
    auth_data->sc->enabled = true;
    auth_data->sc->names[0] = talloc_strdup(auth_data->sc->names, SC1_LABEL);
    assert_non_null(auth_data->sc->names[0]);
    auth_data->sc->names[1] = talloc_strdup(auth_data->sc->names, SC2_LABEL);
    assert_non_null(auth_data->sc->names[1]);
    auth_data->sc->names[2] = NULL;
    auth_data->sc->init_prompt = discard_const(SC_INIT_PROMPT);
    auth_data->sc->pin_prompt = discard_const(SC_PIN_PROMPT);

    will_return(__wrap_json_array_append_new, true);
    ret = json_format_auth_selection(test_ctx, auth_data, &json_msg);
    assert_int_equal(ret, ENOMEM);
    assert_null(json_msg);

    talloc_free(auth_data->sc->names[0]);
    talloc_free(auth_data->sc->names[1]);
    talloc_free(test_ctx);
}

void test_generate_json_message_integration(void **state)
{
    TALLOC_CTX *test_ctx = NULL;
    struct pam_data *pd = NULL;
    struct prompt_config **pc_list = NULL;
    const char *prompt_pin = "true";
    int len;
    int ret;

    test_ctx = talloc_new(NULL);
    assert_non_null(test_ctx);
    pd = talloc_zero(test_ctx, struct pam_data);
    assert_non_null(pd);

    len = strlen(OAUTH2_URI)+1+strlen(OAUTH2_URI_COMP)+1+strlen(OAUTH2_CODE)+1;
    ret = pam_add_response(pd, SSS_PAM_OAUTH2_INFO, len,
                           discard_const(OAUTH2_STR));
    assert_int_equal(ret, EOK);
    len = strlen(SC1_CERT_USER)+1+strlen(SC1_TOKEN_NAME)+1+
          strlen(SC1_MODULE_NAME)+1+strlen(SC1_KEY_ID)+1+strlen(SC1_LABEL)+1+
          strlen(SC1_PROMPT_STR)+1+strlen(SC1_PAM_CERT_USER)+1;
    ret = pam_add_response(pd, SSS_PAM_CERT_INFO, len, discard_const(SC1_STR));
    assert_int_equal(ret, EOK);
    len = strlen(SC2_CERT_USER)+1+strlen(SC2_TOKEN_NAME)+1+
          strlen(SC2_MODULE_NAME)+1+strlen(SC2_KEY_ID)+1+strlen(SC2_LABEL)+1+
          strlen(SC2_PROMPT_STR)+1+strlen(SC2_PAM_CERT_USER)+1;
    ret = pam_add_response(pd, SSS_PAM_CERT_INFO, len, discard_const(SC2_STR));
    assert_int_equal(ret, EOK);
    len = strlen(prompt_pin)+1;
    ret = pam_add_response(pd, SSS_PAM_PASSKEY_INFO, len,
                           discard_const(prompt_pin));
    ret = pc_list_add_password(&pc_list, PASSWORD_PROMPT);
    assert_int_equal(ret, EOK);
    ret = pc_list_add_eidp(&pc_list, OAUTH2_INIT_PROMPT, OAUTH2_LINK_PROMPT);
    assert_int_equal(ret, EOK);
    ret = pc_list_add_passkey(&pc_list, PASSKEY_INIT_PROMPT,
                              PASSKEY_PIN_PROMPT, PASSKEY_TOUCH_PROMPT);
    assert_int_equal(ret, EOK);

    will_return_count(__wrap_json_array_append_new, false, 5);
    ret = generate_json_auth_message(NULL, pc_list, pd);
    assert_int_equal(ret, EOK);
    assert_string_equal((char*) pd->resp_list->data, AUTH_SELECTION_ALL);

    pc_list_free(pc_list);
    talloc_free(test_ctx);
}

void test_json_unpack_password_ok(void **state)
{
    json_t *jroot = NULL;
    char *password = NULL;
    json_error_t jret;
    int ret;

    jroot = json_loads(PASSWORD_CONTENT, 0, &jret);
    assert_non_null(jroot);

    ret = json_unpack_password(jroot, &password);
    assert_int_equal(ret, EOK);
    assert_string_equal(password, "ThePassword");
    json_decref(jroot);
}

void test_json_unpack_pin_ok(void **state)
{
    json_t *jroot = NULL;
    char *pin = NULL;
    json_error_t jret;
    int ret;

    jroot = json_loads(PIN_CONTENT, 0, &jret);
    assert_non_null(jroot);

    ret = json_unpack_pin(jroot, &pin);
    assert_int_equal(ret, EOK);
    assert_string_equal(pin, "ThePIN");
    json_decref(jroot);
}

void test_json_unpack_auth_reply_password(void **state)
{
    TALLOC_CTX *test_ctx = NULL;
    struct pam_data *pd = NULL;
    const char *password = NULL;
    size_t len;
    int ret;

    test_ctx = talloc_new(NULL);
    assert_non_null(test_ctx);
    pd = talloc_zero(test_ctx, struct pam_data);
    assert_non_null(pd);
    pd->authtok = sss_authtok_new(pd);
    assert_non_null(pd->authtok);
    pd->json_auth_selected = discard_const(AUTH_MECH_REPLY_PASSWORD);

    ret = json_unpack_auth_reply(pd);
    assert_int_equal(ret, EOK);
    assert_int_equal(sss_authtok_get_type(pd->authtok), SSS_AUTHTOK_TYPE_PASSWORD);
    sss_authtok_get_password(pd->authtok, &password, &len);
    assert_string_equal(password, "ThePassword");

    talloc_free(test_ctx);
}

void test_json_unpack_auth_reply_oauth2(void **state)
{
    TALLOC_CTX *test_ctx = NULL;
    struct pam_data *pd = NULL;
    const char *code = NULL;
    size_t len;
    int ret;

    test_ctx = talloc_new(NULL);
    assert_non_null(test_ctx);
    pd = talloc_zero(test_ctx, struct pam_data);
    assert_non_null(pd);
    pd->authtok = sss_authtok_new(pd);
    assert_non_null(pd->authtok);
    pd->json_auth_msg = discard_const(AUTH_SELECTION_OAUTH2);
    pd->json_auth_selected = discard_const(AUTH_MECH_REPLY_OAUTH2);

    ret = json_unpack_auth_reply(pd);
    assert_int_equal(ret, EOK);
    assert_int_equal(sss_authtok_get_type(pd->authtok), SSS_AUTHTOK_TYPE_OAUTH2);
    sss_authtok_get_oauth2(pd->authtok, &code, &len);
    assert_string_equal(code, OAUTH2_CODE);

    talloc_free(test_ctx);
}

void test_json_unpack_auth_reply_sc1(void **state)
{
    TALLOC_CTX *test_ctx = NULL;
    struct pam_data *pd = NULL;
    const char *pin = NULL;
    const char *token_name = NULL;
    const char *module_name = NULL;
    const char *key_id = NULL;
    const char *label = NULL;
    size_t len;
    size_t pin_len, token_name_len, module_name_len, key_id_len, label_len;
    int ret;

    test_ctx = talloc_new(NULL);
    assert_non_null(test_ctx);
    pd = talloc_zero(test_ctx, struct pam_data);
    assert_non_null(pd);
    pd->authtok = sss_authtok_new(pd);
    assert_non_null(pd->authtok);
    pd->json_auth_selected = discard_const(AUTH_MECH_REPLY_SMARTCARD);
    len = strlen(SC1_CERT_USER)+1+strlen(SC1_TOKEN_NAME)+1+
          strlen(SC1_MODULE_NAME)+1+strlen(SC1_KEY_ID)+1+strlen(SC1_LABEL)+1+
          strlen(SC1_PROMPT_STR)+1+strlen(SC1_PAM_CERT_USER)+1;
    ret = pam_add_response(pd, SSS_PAM_CERT_INFO, len, discard_const(SC1_STR));
    assert_int_equal(ret, EOK);

    ret = json_unpack_auth_reply(pd);
    assert_int_equal(ret, EOK);
    assert_int_equal(sss_authtok_get_type(pd->authtok), SSS_AUTHTOK_TYPE_SC_PIN);
    ret = sss_authtok_get_sc(pd->authtok, &pin, &pin_len,
                             &token_name, &token_name_len,
                             &module_name, &module_name_len,
                             &key_id, &key_id_len,
                             &label, &label_len);
    assert_int_equal(ret, EOK);
    assert_string_equal(pin, "ThePIN");
    assert_string_equal(token_name, SC1_TOKEN_NAME);
    assert_string_equal(module_name, SC1_MODULE_NAME);
    assert_string_equal(key_id, SC1_KEY_ID);
    assert_string_equal(label, SC1_LABEL);

    talloc_free(test_ctx);
}

void test_json_unpack_auth_reply_sc2(void **state)
{
    TALLOC_CTX *test_ctx = NULL;
    struct pam_data *pd = NULL;
    const char *pin = NULL;
    const char *token_name = NULL;
    const char *module_name = NULL;
    const char *key_id = NULL;
    const char *label = NULL;
    size_t len;
    size_t pin_len, token_name_len, module_name_len, key_id_len, label_len;
    int ret;

    test_ctx = talloc_new(NULL);
    assert_non_null(test_ctx);
    pd = talloc_zero(test_ctx, struct pam_data);
    assert_non_null(pd);
    pd->authtok = sss_authtok_new(pd);
    assert_non_null(pd->authtok);
    pd->json_auth_selected = discard_const(AUTH_MECH_REPLY_SMARTCARD);
    len = strlen(SC1_CERT_USER)+1+strlen(SC1_TOKEN_NAME)+1+
          strlen(SC1_MODULE_NAME)+1+strlen(SC1_KEY_ID)+1+strlen(SC1_LABEL)+1+
          strlen(SC1_PROMPT_STR)+1+strlen(SC1_PAM_CERT_USER)+1;
    ret = pam_add_response(pd, SSS_PAM_CERT_INFO, len, discard_const(SC1_STR));
    assert_int_equal(ret, EOK);
    len = strlen(SC2_CERT_USER)+1+strlen(SC2_TOKEN_NAME)+1+
          strlen(SC2_MODULE_NAME)+1+strlen(SC2_KEY_ID)+1+strlen(SC2_LABEL)+1+
          strlen(SC2_PROMPT_STR)+1+strlen(SC2_PAM_CERT_USER)+1;
    ret = pam_add_response(pd, SSS_PAM_CERT_INFO, len, discard_const(SC2_STR));
    assert_int_equal(ret, EOK);

    ret = json_unpack_auth_reply(pd);
    assert_int_equal(ret, EOK);
    assert_int_equal(sss_authtok_get_type(pd->authtok), SSS_AUTHTOK_TYPE_SC_PIN);
    ret = sss_authtok_get_sc(pd->authtok, &pin, &pin_len,
                             &token_name, &token_name_len,
                             &module_name, &module_name_len,
                             &key_id, &key_id_len,
                             &label, &label_len);
    assert_int_equal(ret, EOK);
    assert_string_equal(pin, "ThePIN");
    assert_string_equal(token_name, SC1_TOKEN_NAME);
    assert_string_equal(module_name, SC1_MODULE_NAME);
    assert_string_equal(key_id, SC1_KEY_ID);
    assert_string_equal(label, SC1_LABEL);

    talloc_free(test_ctx);
}

void test_json_unpack_auth_reply_passkey(void **state)
{
    TALLOC_CTX *test_ctx = NULL;
    struct pam_data *pd = NULL;
    enum sss_authtok_type type;
    const char *pin = NULL;
    char *data = NULL;
    size_t len = 0;
    int ret;

    test_ctx = talloc_new(NULL);
    assert_non_null(test_ctx);
    pd = talloc_zero(test_ctx, struct pam_data);
    assert_non_null(pd);
    pd->authtok = sss_authtok_new(pd);
    assert_non_null(pd->authtok);
    type = SSS_AUTHTOK_TYPE_PASSKEY;
    data = talloc_strdup(test_ctx, "passkey");
    assert_non_null(data);
    len = strlen(data) + 1;
    ret = sss_authtok_set(pd->authtok, type, (const uint8_t *)data, len);
    pd->json_auth_msg = discard_const(AUTH_SELECTION_PASSKEY);
    pd->json_auth_selected = discard_const(AUTH_MECH_REPLY_PASSKEY);

    ret = json_unpack_auth_reply(pd);
    assert_int_equal(ret, EOK);
    assert_int_equal(sss_authtok_get_type(pd->authtok), SSS_AUTHTOK_TYPE_PASSKEY);
    sss_authtok_get_passkey_pin(pd->authtok, &pin, &len);
    assert_string_equal(pin, "ThePIN");

    talloc_free(test_ctx);
}

void test_json_unpack_auth_reply_failure(void **state)
{
    TALLOC_CTX *test_ctx = NULL;
    struct pam_data *pd = NULL;
    int ret;

    test_ctx = talloc_new(NULL);
    assert_non_null(test_ctx);
    pd = talloc_zero(test_ctx, struct pam_data);
    assert_non_null(pd);
    pd->json_auth_selected = discard_const(AUTH_MECH_ERRONEOUS);

    ret = json_unpack_auth_reply(pd);
    assert_int_equal(ret, EINVAL);

    talloc_free(test_ctx);
}

void test_json_unpack_oauth2_code(void **state)
{
    TALLOC_CTX *test_ctx = NULL;
    char *oauth2_code = NULL;
    int ret;

    test_ctx = talloc_new(NULL);
    assert_non_null(test_ctx);

    ret = json_unpack_oauth2_code(test_ctx, discard_const(AUTH_SELECTION_ALL),
                                  &oauth2_code);
    assert_int_equal(ret, EOK);
    assert_string_equal(oauth2_code, OAUTH2_CODE);

    talloc_free(test_ctx);
}

void test_is_pam_json_enabled_service_in_list(void **state)
{
    char *json_services[] = {discard_const("sshd"), discard_const("su"),
                             discard_const("gdm-switchable-auth"), NULL};
    bool result;

    result = is_pam_json_enabled(json_services,
                                 discard_const("gdm-switchable-auth"));
    assert_int_equal(result, true);
}

void test_is_pam_json_enabled_service_not_in_list(void **state)
{
    char *json_services[] = {discard_const("sshd"), discard_const("su"),
                             discard_const("gdm-switchable-auth"), NULL};
    bool result;

    result = is_pam_json_enabled(json_services,
                                 discard_const("sudo"));
    assert_int_equal(result, false);
}

void test_is_pam_json_enabled_null_list(void **state)
{
    bool result;

    result = is_pam_json_enabled(NULL,
                                 discard_const("sudo"));
    assert_int_equal(result, false);
}

static void test_parse_supp_valgrind_args(void)
{
    /*
     * The objective of this function is to filter the unit-test functions
     * that trigger a valgrind memory leak and suppress them to avoid false
     * positives.
     */
    DEBUG_CLI_INIT(debug_level);
}

int main(int argc, const char *argv[])
{
    poptContext pc;
    int opt;
    struct poptOption long_options[] = {
        POPT_AUTOHELP
        SSSD_DEBUG_OPTS
        POPT_TABLEEND
    };

    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_get_cert_list),
        cmocka_unit_test_setup_teardown(test_get_cert_names, setup, teardown),
        cmocka_unit_test_setup_teardown(test_json_format_mechanisms_password, setup, teardown),
        cmocka_unit_test_setup_teardown(test_json_format_mechanisms_oauth2, setup, teardown),
        cmocka_unit_test_setup_teardown(test_json_format_mechanisms_sc1, setup, teardown),
        cmocka_unit_test_setup_teardown(test_json_format_mechanisms_sc2, setup, teardown),
        cmocka_unit_test_setup_teardown(test_json_format_mechanisms_passkey, setup, teardown),
        cmocka_unit_test_setup_teardown(test_json_format_priority_all, setup, teardown),
        cmocka_unit_test_setup_teardown(test_json_format_auth_selection_password, setup, teardown),
        cmocka_unit_test_setup_teardown(test_json_format_auth_selection_oauth2, setup, teardown),
        cmocka_unit_test_setup_teardown(test_json_format_auth_selection_sc, setup, teardown),
        cmocka_unit_test_setup_teardown(test_json_format_auth_selection_passkey, setup, teardown),
        cmocka_unit_test_setup_teardown(test_json_format_auth_selection_all, setup, teardown),
        cmocka_unit_test_setup_teardown(test_json_format_auth_selection_failure, setup, teardown),
        cmocka_unit_test(test_generate_json_message_integration),
        cmocka_unit_test(test_json_unpack_password_ok),
        cmocka_unit_test(test_json_unpack_pin_ok),
        cmocka_unit_test(test_json_unpack_auth_reply_password),
        cmocka_unit_test(test_json_unpack_auth_reply_oauth2),
        cmocka_unit_test(test_json_unpack_auth_reply_sc1),
        cmocka_unit_test(test_json_unpack_auth_reply_sc2),
        cmocka_unit_test(test_json_unpack_auth_reply_passkey),
        cmocka_unit_test(test_json_unpack_auth_reply_failure),
        cmocka_unit_test(test_json_unpack_oauth2_code),
        cmocka_unit_test(test_is_pam_json_enabled_service_in_list),
        cmocka_unit_test(test_is_pam_json_enabled_service_not_in_list),
        cmocka_unit_test(test_is_pam_json_enabled_null_list),
    };

    /* Set debug level to invalid value so we can decide if -d 0 was used. */
    debug_level = SSSDBG_INVALID;

    pc = poptGetContext(argv[0], argc, argv, long_options, 0);
    while((opt = poptGetNextOpt(pc)) != -1) {
        switch(opt) {
        default:
            fprintf(stderr, "\nInvalid option %s: %s\n\n",
                    poptBadOption(pc, 0), poptStrerror(opt));
            poptPrintUsage(pc, stderr, 0);
            return 1;
        }
    }
    poptFreeContext(pc);

    test_parse_supp_valgrind_args();

    /* Even though normally the tests should clean up after themselves
     * they might not after a failed run. Remove the old DB to be sure */
    tests_set_cwd();

    return cmocka_run_group_tests(tests, NULL, NULL);
}
