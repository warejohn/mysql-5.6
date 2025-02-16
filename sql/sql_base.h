/* Copyright (c) 2010, 2017, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

#ifndef SQL_BASE_INCLUDED
#define SQL_BASE_INCLUDED

#include "unireg.h"                    // REQUIRED: for other includes
#include "sql_trigger.h"                        /* trg_event_type */
#include "sql_class.h"                          /* enum_mark_columns */
#include "mysqld.h"                             /* key_map */

class Item_ident;
struct Name_resolution_context;
class Open_table_context;
class Open_tables_state;
class Prelocking_strategy;
struct TABLE_LIST;
class THD;
struct handlerton;
struct TABLE;

typedef class st_select_lex SELECT_LEX;

typedef struct st_lock_param_type ALTER_PARTITION_PARAM_TYPE;

/*
  This enumeration type is used only by the function find_item_in_list
  to return the info on how an item has been resolved against a list
  of possibly aliased items.
  The item can be resolved:
   - against an alias name of the list's element (RESOLVED_AGAINST_ALIAS)
   - against non-aliased field name of the list  (RESOLVED_WITH_NO_ALIAS)
   - against an aliased field name of the list   (RESOLVED_BEHIND_ALIAS)
   - ignoring the alias name in cases when SQL requires to ignore aliases
     (e.g. when the resolved field reference contains a table name or
     when the resolved item is an expression)   (RESOLVED_IGNORING_ALIAS)
*/
enum enum_resolution_type {
  NOT_RESOLVED=0,
  RESOLVED_IGNORING_ALIAS,
  RESOLVED_BEHIND_ALIAS,
  RESOLVED_WITH_NO_ALIAS,
  RESOLVED_AGAINST_ALIAS
};

enum find_item_error_report_type {REPORT_ALL_ERRORS, REPORT_EXCEPT_NOT_FOUND,
				  IGNORE_ERRORS, REPORT_EXCEPT_NON_UNIQUE,
                                  IGNORE_EXCEPT_NON_UNIQUE};

enum enum_tdc_remove_table_type {TDC_RT_REMOVE_ALL, TDC_RT_REMOVE_NOT_OWN,
                                 TDC_RT_REMOVE_UNUSED,
                                 TDC_RT_REMOVE_NOT_OWN_KEEP_SHARE};

/* bits for last argument to remove_table_from_cache() */
#define RTFC_NO_FLAG                0x0000
#define RTFC_OWNED_BY_THD_FLAG      0x0001
#define RTFC_WAIT_OTHER_THREAD_FLAG 0x0002
#define RTFC_CHECK_KILLED_FLAG      0x0004

// A wrapper around std::stoul that handles exceptions
int stoul_noexcept(const std::string str, ulong *val);

bool check_dup(const char *db, const char *name, TABLE_LIST *tables);
extern mysql_mutex_t LOCK_open;
bool table_cache_init(void);
void table_cache_free(void);
bool table_def_init(void);
void table_def_free(void);
void table_def_start_shutdown(void);
void assign_new_table_id(TABLE_SHARE *share);
uint cached_table_definitions(void);
uint get_table_def_key(const TABLE_LIST *table_list, const char **key);
TABLE_SHARE *get_table_share(THD *thd, TABLE_LIST *table_list,
                             const char *key, uint key_length,
                             uint db_flags, int *error,
                             my_hash_value_type hash_value);
void release_table_share(TABLE_SHARE *share);
TABLE_SHARE *get_cached_table_share(const char *db, const char *table_name);

TABLE *open_ltable(THD *thd, TABLE_LIST *table_list, thr_lock_type update,
                   uint lock_flags);

/* mysql_lock_tables() and open_table() flags bits */
#define MYSQL_OPEN_IGNORE_GLOBAL_READ_LOCK      0x0001
#define MYSQL_OPEN_IGNORE_FLUSH                 0x0002
/* MYSQL_OPEN_TEMPORARY_ONLY (0x0004) is not used anymore. */
#define MYSQL_LOCK_IGNORE_GLOBAL_READ_ONLY      0x0008
#define MYSQL_LOCK_LOG_TABLE                    0x0010
/**
  Do not try to acquire a metadata lock on the table: we
  already have one.
*/
#define MYSQL_OPEN_HAS_MDL_LOCK                 0x0020
/**
  If in locked tables mode, ignore the locked tables and get
  a new instance of the table.
*/
#define MYSQL_OPEN_GET_NEW_TABLE                0x0040
/* 0x0080 used to be MYSQL_OPEN_SKIP_TEMPORARY */
/** Fail instead of waiting when conficting metadata lock is discovered. */
#define MYSQL_OPEN_FAIL_ON_MDL_CONFLICT         0x0100
/** Open tables using MDL_SHARED lock instead of one specified in parser. */
#define MYSQL_OPEN_FORCE_SHARED_MDL             0x0200
/**
  Open tables using MDL_SHARED_HIGH_PRIO lock instead of one specified
  in parser.
*/
#define MYSQL_OPEN_FORCE_SHARED_HIGH_PRIO_MDL   0x0400
/**
  When opening or locking the table, use the maximum timeout
  (LONG_TIMEOUT = 1 year) rather than the user-supplied timeout value.
*/
#define MYSQL_LOCK_IGNORE_TIMEOUT               0x0800
/**
  When acquiring "strong" (SNW, SNRW, X) metadata locks on tables to
  be open do not acquire global and schema-scope IX locks.
*/
#define MYSQL_OPEN_SKIP_SCOPED_MDL_LOCK         0x1000
/**
  When opening or locking a replication table through an internal
  operation rather than explicitly through an user thread.
*/
#define MYSQL_LOCK_RPL_INFO_TABLE               0x2000
/**
  Only check THD::killed if waits happen (e.g. wait on MDL, wait on
  table flush, wait on thr_lock.c locks) while opening and locking table.
*/
#define MYSQL_OPEN_IGNORE_KILLED                0x4000

/** Please refer to the internals manual. */
#define MYSQL_OPEN_REOPEN  (MYSQL_OPEN_IGNORE_FLUSH |\
                            MYSQL_OPEN_IGNORE_GLOBAL_READ_LOCK |\
                            MYSQL_LOCK_IGNORE_GLOBAL_READ_ONLY |\
                            MYSQL_LOCK_IGNORE_TIMEOUT |\
                            MYSQL_OPEN_IGNORE_KILLED |\
                            MYSQL_OPEN_GET_NEW_TABLE |\
                            MYSQL_OPEN_HAS_MDL_LOCK)

bool open_table(THD *thd, TABLE_LIST *table_list, Open_table_context *ot_ctx);

bool get_key_map_from_key_list(key_map *map, TABLE *table,
                               List<String> *index_list);
TABLE *open_table_uncached(THD *thd, const char *path, const char *db,
			   const char *table_name,
                           bool add_to_temporary_tables_list,
                           bool open_in_engine);
TABLE *find_locked_table(TABLE *list, const char *db, const char *table_name);
thr_lock_type read_lock_type_for_table(THD *thd,
                                       Query_tables_list *prelocking_ctx,
                                       TABLE_LIST *table_list,
                                       bool routine_modifies_data);

my_bool mysql_rm_tmp_tables(void);
bool rm_temporary_table(handlerton *base, const char *path);
void close_tables_for_reopen(THD *thd, TABLE_LIST **tables,
                             const MDL_savepoint &start_of_statement_svp);
TABLE_LIST *find_table_in_list(TABLE_LIST *table,
                               TABLE_LIST *TABLE_LIST::*link,
                               const char *db_name,
                               const char *table_name);
TABLE *find_temporary_table(THD *thd, const char *db, const char *table_name);
TABLE *find_temporary_table(THD *thd, const TABLE_LIST *tl);
TABLE *find_temporary_table(THD *thd, const char *table_key,
                            uint table_key_length);
void close_thread_tables(THD *thd, bool async_commit=FALSE);
bool fill_record_n_invoke_before_triggers(THD *thd, List<Item> &fields,
                                          List<Item> &values,
                                          bool ignore_errors,
                                          Table_triggers_list *triggers,
                                          enum trg_event_type event);
bool fill_record_n_invoke_before_triggers(THD *thd, Field **field,
                                          List<Item> &values,
                                          bool ignore_errors,
                                          Table_triggers_list *triggers,
                                          enum trg_event_type event);
bool insert_fields(THD *thd, Name_resolution_context *context,
		   const char *db_name, const char *table_name,
                   List_iterator<Item> *it, bool any_privileges, int keyno);
int setup_wild(THD *thd, TABLE_LIST *tables, List<Item> &fields,
	       List<Item> *sum_func_list, uint wild_num);
bool setup_fields(THD *thd, Ref_ptr_array ref_pointer_array,
                  List<Item> &item, enum_mark_columns mark_used_columns,
                  List<Item> *sum_func_list, bool allow_sum_func);
bool fill_record(THD * thd, List<Item> &fields, List<Item> &values,
                 bool ignore_errors, MY_BITMAP *bitmap);
bool fill_record(THD *thd, Field **field, List<Item> &values,
                 bool ignore_errors, MY_BITMAP *bitmap);

Field *
find_field_in_tables(THD *thd, Item_ident *item,
                     TABLE_LIST *first_table, TABLE_LIST *last_table,
                     Item **ref, find_item_error_report_type report_error,
                     bool check_privileges, bool register_tree_change);
Field *
find_field_in_table_ref(THD *thd, TABLE_LIST *table_list,
                        const char *name, uint length,
                        const char *item_name, const char *db_name,
                        const char *table_name, Item **ref,
                        bool check_privileges, bool allow_rowid,
                        uint *cached_field_index_ptr,
                        bool register_tree_change, TABLE_LIST **actual_table,
                        bool is_document_path = false);
Field *
find_field_in_table(THD *thd, TABLE *table, const char *name, uint length,
                    bool allow_rowid, uint *cached_field_index_ptr);
Field *
find_field_in_table_sef(TABLE *table, const char *name);
Item ** find_item_in_list(Item *item, List<Item> &items, uint *counter,
                          find_item_error_report_type report_error,
                          enum_resolution_type *resolution);
bool setup_tables(THD *thd, Name_resolution_context *context,
                  List<TABLE_LIST> *from_clause, TABLE_LIST *tables,
                  TABLE_LIST **leaves, bool select_insert);
bool setup_tables_and_check_access(THD *thd,
                                   Name_resolution_context *context,
                                   List<TABLE_LIST> *from_clause,
                                   TABLE_LIST *tables,
                                   TABLE_LIST **leaves,
                                   bool select_insert,
                                   ulong want_access_first,
                                   ulong want_access);
bool wait_while_table_is_used(THD *thd, TABLE *table,
                              enum ha_extra_function function);

void drop_open_table(THD *thd, TABLE *table, const char *db_name,
                     const char *table_name);
void update_non_unique_table_error(TABLE_LIST *update,
                                   const char *operation,
                                   TABLE_LIST *duplicate);
int setup_conds(THD *thd, TABLE_LIST *tables, TABLE_LIST *leaves,
		Item **conds);
int setup_ftfuncs(SELECT_LEX* select);
bool init_ftfuncs(THD *thd, SELECT_LEX* select, bool no_order);
bool lock_table_names_nsec(THD *thd, TABLE_LIST *table_list,
                      TABLE_LIST *table_list_end,
                      ulonglong lock_wait_timeout_nsec,
                      uint flags);
bool open_tables(THD *thd, TABLE_LIST **tables, uint *counter, uint flags,
                 Prelocking_strategy *prelocking_strategy);
bool get_table_from_cache(THD *thd, TABLE_LIST *table);
void return_table_to_cache(THD *thd, TABLE_LIST *table);
/* open_and_lock_tables with optional derived handling */
bool open_and_lock_tables(THD *thd, TABLE_LIST *tables,
                          bool derived, uint flags,
                          Prelocking_strategy *prelocking_strategy);
/* simple open_and_lock_tables without derived handling for single table */
TABLE *open_n_lock_single_table(THD *thd, TABLE_LIST *table_l,
                                thr_lock_type lock_type, uint flags,
                                Prelocking_strategy *prelocking_strategy);
bool open_normal_and_derived_tables(THD *thd, TABLE_LIST *tables, uint flags);
bool lock_tables(THD *thd, TABLE_LIST *tables, uint counter, uint flags);
void free_io_cache(TABLE *entry);
void intern_close_table(TABLE *entry);
void close_thread_table(THD *thd, TABLE **table_ptr, bool update_stats);
bool close_temporary_tables(THD *thd);
TABLE_LIST *unique_table(THD *thd, TABLE_LIST *table, TABLE_LIST *table_list,
                         bool check_alias);
int drop_temporary_table(THD *thd, TABLE_LIST *table_list, bool *is_trans);
void close_temporary_table(THD *thd, TABLE *table, bool free_share,
                           bool delete_table);
void close_temporary(THD* thd, TABLE *table, bool free_share,
		     bool delete_table);
bool rename_temporary_table(THD* thd, TABLE *table, const char *new_db,
			    const char *table_name);
bool open_temporary_tables(THD *thd, TABLE_LIST *tl_list);
bool open_temporary_table(THD *thd, TABLE_LIST *tl);
bool is_equal(const LEX_STRING *a, const LEX_STRING *b);

/* ########### begin of name encoding service ########### */

/*
** init_names
**
** Initialize the name maps for all object names we care about
** Called at server startup
*/
void init_names();

/*
** destroy_names
**
** Destroy the name maps. Called server shutdown
*/
void destroy_names();

/*
** fill_invert_map
** Populate id-to-name map from name-to-id map to allow efficient
** lookup of names based on an ID when fetching data for tables
** *_STATISTICS
*/
bool fill_invert_map(enum_map_name map_name, ID_NAME_MAP *id_map);

/*
** get_id
**
** Returns the ID for a given name (db, table, etc).
** If the name is already in the map then it returns its ID, else
** it'll either reuse the ID of a dropped name or create a new ID
** It returns INVALID_NAME_ID in case of an exception, e.g, if we
** reach the maximum capacity of the map
*/
#define INVALID_NAME_ID UINT_MAX
uint get_id(enum_map_name map_name, const char *name, uint length);

const char *get_name(ID_NAME_MAP *id_map, uint id);

/* ########### end of name encoding service ########### */

/* for SHOW GLOBAL TABLE STATUS */
/* for information_schema.table_statistics and index_statistics */
void update_table_stats(THD* thd, TABLE *table_ptr, bool follow_next);
extern HASH global_table_stats;
extern mysql_mutex_t LOCK_global_table_stats;
void init_global_table_stats(void);
void free_global_table_stats(void);
void reset_global_table_stats(void);
bool lock_global_table_stats(void);
void unlock_global_table_stats(bool acquired);
extern ST_FIELD_INFO table_stats_fields_info[];
extern ST_FIELD_INFO index_stats_fields_info[];
int fill_table_stats(THD *thd, TABLE_LIST *tables, Item *cond);
int fill_index_stats(THD *thd, TABLE_LIST *tables, Item *cond);
typedef void (*table_stats_cb)(const char *db, const char *table,
			       bool is_partition,
			       my_io_perf_t* r, my_io_perf_t* w,
			       my_io_perf_t *r_blob,
			       my_io_perf_t *r_primary,
			       my_io_perf_t *r_secondary,
			       page_stats_t* page_stats,
			       comp_stats_t* comp_stats,
			       int n_lock_wait,
			       int n_lock_wait_timeout,
			       int n_lock_deadlock,
			       const char *engine);

/* for information_schema.db_statistics */
extern ST_FIELD_INFO db_stats_fields_info[];
void init_global_db_stats(void);
void free_global_db_stats(void);
void reset_global_db_stats(void);
int fill_db_stats(THD *thd, TABLE_LIST *tables, Item *cond);

/* for information_schema.user_table_statistics */
extern ST_FIELD_INFO user_table_stats_fields_info[];
void free_table_stats_for_user(USER_CONN *user_conn);
void free_table_stats_for_all_users();
void init_table_stats_for_user(USER_CONN *user_conn);
void delete_user_table_stats(const char *old_name);
void rename_user_table_stats(const char *old_name, const char *new_name);
void reset_table_stats_for_user(USER_CONN *user_conn);
int  fill_user_table_stats(THD *thd, TABLE_LIST *tables, Item *cond);
bool valid_user_table_stats(USER_TABLE_STATS *stats);
bool fill_shared_table_stats(SHARED_TABLE_STATS *stats, TABLE *table,
                             int *offset,
                             std::array<ID_NAME_MAP*, 2> *i_map);
bool check_admin_users_list(USER_CONN *uc);
void reset_user_conn_admin_flag();

/* For information_schema.sql_statistics */
extern ST_FIELD_INFO sql_stats_fields_info[];
extern mysql_mutex_t LOCK_global_sql_stats;
void init_global_sql_stats();
void free_global_sql_stats(bool limits_updated);
int  fill_sql_stats(THD *thd, TABLE_LIST *tables, Item *cond);
void update_sql_stats_after_statement(THD *thd, SHARED_SQL_STATS *stats, char *sub_query);
void reset_sql_stats_from_thd(THD *thd, SHARED_SQL_STATS *stats);
void reset_sql_stats_from_diff(THD *thd, SHARED_SQL_STATS *prev_stats,
                               SHARED_SQL_STATS *stats);
bool is_sql_stats_collection_above_limit();
bool toggle_sql_stats_snapshot(THD *thd);
void flush_sql_statistics(THD *thd);

/* For active sql */
extern mysql_mutex_t LOCK_global_active_sql;
void free_global_active_sql(void);
bool register_active_sql(THD *thd, char *query_text, uint query_length);
void remove_active_sql(THD *thd);

/* For information_schema.sql_text */
extern ST_FIELD_INFO sql_text_fields_info[];
int fill_sql_text(THD *thd, TABLE_LIST *tables, Item *cond);

extern ST_FIELD_INFO client_attrs_fields_info[];
int fill_client_attrs(THD *thd, TABLE_LIST *tables, Item *cond);

/* Stores the client attribute names */
void store_client_attribute_names(char *new_value);

/* For information_schema.sql_plans */
extern ST_FIELD_INFO sql_plan_fields_info[];
extern mysql_mutex_t LOCK_global_sql_plans;
void free_global_sql_plans(void);
int  fill_sql_plans(THD *thd, TABLE_LIST *tables, Item *cond);
void insert_sql_plan(THD *thd, String *json_plan);

/* For information_schema.sql_findings */
extern ST_FIELD_INFO sql_findings_fields_info[];
extern mysql_mutex_t LOCK_global_sql_findings;
int  fill_sql_findings(THD *thd, TABLE_LIST *tables, Item *cond);
void free_global_sql_findings(void);
void store_sql_findings(THD *thd, char *query_text);

/* For information_schema.write_statistics */
extern ST_FIELD_INFO write_statistics_fields_info[];
extern mysql_mutex_t LOCK_global_write_statistics;
int  fill_write_statistics(THD *thd, TABLE_LIST *tables, Item *cond);
void free_global_write_statistics(void);
void store_write_statistics(THD *thd);

/* For information_schema.write_throttling_rules */
extern ST_FIELD_INFO write_throttling_rules_fields_info[];
extern mysql_mutex_t LOCK_global_write_throttling_rules;
int  fill_write_throttling_rules(THD *thd, TABLE_LIST *tables, Item *cond);
void free_global_write_throttling_rules(void);
void free_global_write_auto_throttling_rules(void);
bool store_write_throttling_rules(THD *thd);
void check_lag_and_throttle(time_t time_now);
bool store_write_throttle_permissible_dimensions_in_order(char *new_value);

/* For information_schema.write_throttling_log */
extern ST_FIELD_INFO write_throttling_log_fields_info[];
extern mysql_mutex_t LOCK_global_write_throttling_log;
int  fill_write_throttling_log(THD *thd, TABLE_LIST *tables, Item *cond);
void store_write_throttling_log(THD *thd, int type, std::string value, WRITE_THROTTLING_RULE &rule);
void store_long_qry_abort_log(THD *thd);
void free_global_write_throttling_log(void);

/* For information_schema.COLUMN_STATISTICS */
extern ST_FIELD_INFO column_statistics_fields_info[];
int fill_column_statistics(THD *thd, TABLE_LIST *tables, Item *cond);

/* For information_schema.REPLICA_STATISTICS */
extern ST_FIELD_INFO replica_statistics_fields_info[];
int fill_replica_statistics(THD *thd, TABLE_LIST *tables, Item *cond);

/* For information_schema.TRANSACTION_SIZE_HISTOGRAM */
extern ST_FIELD_INFO tx_size_histogram_fields_info[];
extern mysql_mutex_t LOCK_global_tx_size_histogram;
int  fill_tx_size_histogram(THD *thd, TABLE_LIST *tables, Item *cond);
void free_global_tx_size_histogram();
void update_tx_size_histogram(THD *thd);

/* For information_schema.WRITE_STATISTICS_HISTOGRAM */
extern ST_FIELD_INFO write_stat_histogram_fields_info[];
extern mysql_mutex_t LOCK_global_write_stat_histogram;
int  fill_write_stat_histogram(THD *thd, TABLE_LIST *tables, Item *cond);
void reset_write_stat_histogram();

/* For information_schema.user_latency_histograms */
extern char *histogram_step_size_connection_create;
extern char *histogram_step_size_update_command;
extern char *histogram_step_size_delete_command;
extern char *histogram_step_size_insert_command;
extern char *histogram_step_size_select_command;
extern char *histogram_step_size_ddl_command;
extern char *histogram_step_size_transaction_command;
extern char *histogram_step_size_handler_command;
extern char *histogram_step_size_other_command;
int fill_user_histograms(THD *thd, TABLE_LIST *tables, Item *cond);

/* For information_schema.user_statistics */
extern ST_FIELD_INFO user_stats_fields_info[];
void init_user_stats(USER_STATS *user_stats);
void reset_global_user_stats();
int fill_user_stats(THD *thd, TABLE_LIST *tables, Item *cond);
void
update_user_stats_after_statement(USER_STATS *us,
                                  THD *thd,
                                  ulonglong wall_time,
                                  bool is_other_command,
                                  bool is_xid_event,
                                  my_io_perf_t *start_perf_read,
                                  my_io_perf_t *start_perf_read_blob,
                                  my_io_perf_t *start_perf_read_primary,
                                  my_io_perf_t *start_perf_read_secondary);
void
update_db_stats_after_statement(DB_STATS *dbstats,
                                  THD *thd,
                                  bool is_xid_event);

/* For information_schema.error_statistics */
extern ST_FIELD_INFO error_stats_fields_info[];
void init_global_error_stats(void);
void reset_global_error_stats(void);
void update_error_stats(uint error);
int fill_error_stats(THD *thd, TABLE_LIST *tables, Item *cond);

/* Functions to work with system tables. */
bool open_system_tables_for_read(THD *thd, TABLE_LIST *table_list,
                                 Open_tables_backup *backup);
void close_system_tables(THD *thd, Open_tables_backup *backup);
void close_mysql_tables(THD *thd);
TABLE *open_system_table_for_update(THD *thd, TABLE_LIST *one_table);
TABLE *open_log_table(THD *thd, TABLE_LIST *one_table, Open_tables_backup *backup);
void close_log_table(THD *thd, Open_tables_backup *backup);

TABLE *open_performance_schema_table(THD *thd, TABLE_LIST *one_table,
                                     Open_tables_state *backup);
void close_performance_schema_table(THD *thd, Open_tables_state *backup);

bool close_cached_tables_nsec(THD *thd, TABLE_LIST *tables,
                              bool wait_for_refresh,
                              ulonglong timeout_nsec);
bool close_cached_connection_tables(THD *thd, LEX_STRING *connect_string);
void close_all_tables_for_name(THD *thd, TABLE_SHARE *share,
                               bool remove_from_locked_tables,
                               TABLE *skip_table);
OPEN_TABLE_LIST *list_open_tables(THD *thd, const char *db, const char *wild);
void tdc_remove_table(THD *thd, enum_tdc_remove_table_type remove_type,
                      const char *db, const char *table_name,
                      bool has_lock);
bool tdc_open_view(THD *thd, TABLE_LIST *table_list, const char *alias,
                   const char *cache_key, uint cache_key_length, uint flags);
void tdc_flush_unused_tables();
TABLE *find_table_for_mdl_upgrade(THD *thd, const char *db,
                                  const char *table_name,
                                  bool no_error);
void mark_tmp_table_for_reuse(TABLE *table);
bool check_if_table_exists(THD *thd, TABLE_LIST *table, bool *exists);

extern Item **not_found_item;
extern Field *not_found_field;
extern Field *view_ref_found;
extern HASH table_def_cache;

/**
  clean/setup table fields and map.

  @param table        TABLE structure pointer (which should be setup)
  @param table_list   TABLE_LIST structure pointer (owner of TABLE)
  @param tablenr     table number
*/


inline void setup_table_map(TABLE *table, TABLE_LIST *table_list, uint tablenr)
{
  table->used_fields= 0;
  table->const_table= 0;
  table->null_row= 0;
  table->status= STATUS_GARBAGE | STATUS_NOT_FOUND;
  table->maybe_null= table_list->outer_join;
  TABLE_LIST *embedding= table_list->embedding;
  while (!table->maybe_null && embedding)
  {
    table->maybe_null= embedding->outer_join;
    embedding= embedding->embedding;
  }
  table->tablenr= tablenr;
  table->map= (table_map) 1 << tablenr;
  table->force_index= table_list->force_index;
  table->force_index_order= table->force_index_group= 0;
  table->covering_keys= table->s->keys_for_keyread;
  table->merge_keys.clear_all();
}

inline TABLE_LIST *find_table_in_global_list(TABLE_LIST *table,
                                             const char *db_name,
                                             const char *table_name)
{
  return find_table_in_list(table, &TABLE_LIST::next_global,
                            db_name, table_name);
}

inline TABLE_LIST *find_table_in_local_list(TABLE_LIST *table,
                                            const char *db_name,
                                            const char *table_name)
{
  return find_table_in_list(table, &TABLE_LIST::next_local,
                            db_name, table_name);
}


inline bool setup_fields_with_no_wrap(THD *thd, Ref_ptr_array ref_pointer_array,
                                      List<Item> &item,
                                      enum_mark_columns mark_used_columns,
                                      List<Item> *sum_func_list,
                                      bool allow_sum_func)
{
  bool res;
  thd->lex->select_lex.no_wrap_view_item= TRUE;
  res= setup_fields(thd, ref_pointer_array, item, mark_used_columns,
                    sum_func_list, allow_sum_func);
  thd->lex->select_lex.no_wrap_view_item= FALSE;
  return res;
}

/**
  An abstract class for a strategy specifying how the prelocking
  algorithm should extend the prelocking set while processing
  already existing elements in the set.
*/

class Prelocking_strategy
{
public:
  virtual ~Prelocking_strategy() { }

  virtual bool handle_routine(THD *thd, Query_tables_list *prelocking_ctx,
                              Sroutine_hash_entry *rt, sp_head *sp,
                              bool *need_prelocking) = 0;
  virtual bool handle_table(THD *thd, Query_tables_list *prelocking_ctx,
                            TABLE_LIST *table_list, bool *need_prelocking) = 0;
  virtual bool handle_view(THD *thd, Query_tables_list *prelocking_ctx,
                           TABLE_LIST *table_list, bool *need_prelocking)= 0;
};


/**
  A Strategy for prelocking algorithm suitable for DML statements.

  Ensures that all tables used by all statement's SF/SP/triggers and
  required for foreign key checks are prelocked and SF/SPs used are
  cached.
*/

class DML_prelocking_strategy : public Prelocking_strategy
{
public:
  virtual bool handle_routine(THD *thd, Query_tables_list *prelocking_ctx,
                              Sroutine_hash_entry *rt, sp_head *sp,
                              bool *need_prelocking);
  virtual bool handle_table(THD *thd, Query_tables_list *prelocking_ctx,
                            TABLE_LIST *table_list, bool *need_prelocking);
  virtual bool handle_view(THD *thd, Query_tables_list *prelocking_ctx,
                           TABLE_LIST *table_list, bool *need_prelocking);
};


/**
  A strategy for prelocking algorithm to be used for LOCK TABLES
  statement.
*/

class Lock_tables_prelocking_strategy : public DML_prelocking_strategy
{
  virtual bool handle_table(THD *thd, Query_tables_list *prelocking_ctx,
                            TABLE_LIST *table_list, bool *need_prelocking);
};


/**
  Strategy for prelocking algorithm to be used for ALTER TABLE statements.

  Unlike DML or LOCK TABLES strategy, it doesn't
  prelock triggers, views or stored routines, since they are not
  used during ALTER.
*/

class Alter_table_prelocking_strategy : public Prelocking_strategy
{
public:
  virtual bool handle_routine(THD *thd, Query_tables_list *prelocking_ctx,
                              Sroutine_hash_entry *rt, sp_head *sp,
                              bool *need_prelocking);
  virtual bool handle_table(THD *thd, Query_tables_list *prelocking_ctx,
                            TABLE_LIST *table_list, bool *need_prelocking);
  virtual bool handle_view(THD *thd, Query_tables_list *prelocking_ctx,
                           TABLE_LIST *table_list, bool *need_prelocking);
};


inline bool
open_tables(THD *thd, TABLE_LIST **tables, uint *counter, uint flags)
{
  DML_prelocking_strategy prelocking_strategy;

  return open_tables(thd, tables, counter, flags, &prelocking_strategy);
}


inline TABLE *open_n_lock_single_table(THD *thd, TABLE_LIST *table_l,
                                       thr_lock_type lock_type, uint flags)
{
  DML_prelocking_strategy prelocking_strategy;

  return open_n_lock_single_table(thd, table_l, lock_type, flags,
                                  &prelocking_strategy);
}


/* open_and_lock_tables with derived handling */
inline bool open_and_lock_tables(THD *thd, TABLE_LIST *tables,
                                 bool derived, uint flags)
{
  DML_prelocking_strategy prelocking_strategy;

  return open_and_lock_tables(thd, tables, derived, flags,
                              &prelocking_strategy);
}


/**
  A context of open_tables() function, used to recover
  from a failed open_table() or open_routine() attempt.
*/

class Open_table_context
{
public:
  enum enum_open_table_action
  {
    OT_NO_ACTION= 0,
    OT_BACKOFF_AND_RETRY,
    OT_REOPEN_TABLES,
    OT_DISCOVER,
    OT_REPAIR
  };
  Open_table_context(THD *thd, uint flags);

  bool recover_from_failed_open();
  bool request_backoff_action(enum_open_table_action action_arg,
                              TABLE_LIST *table);

  bool can_recover_from_failed_open() const
  { return m_action != OT_NO_ACTION; }

  /**
    When doing a back-off, we close all tables acquired by this
    statement.  Return an MDL savepoint taken at the beginning of
    the statement, so that we can rollback to it before waiting on
    locks.
  */
  const MDL_savepoint &start_of_statement_svp() const
  {
    return m_start_of_statement_svp;
  }

  inline ulong get_timeout_nsec() const
  {
    return m_timeout_nsec;
  }

  uint get_flags() const { return m_flags; }

  /**
    Set flag indicating that we have already acquired metadata lock
    protecting this statement against GRL while opening tables.
  */
  void set_has_protection_against_grl()
  {
    m_has_protection_against_grl= TRUE;
  }

  bool has_protection_against_grl() const
  {
    return m_has_protection_against_grl;
  }

private:
  /* THD for which tables are opened. */
  THD *m_thd;
  /**
    For OT_DISCOVER and OT_REPAIR actions, the table list element for
    the table which definition should be re-discovered or which
    should be repaired.
  */
  TABLE_LIST *m_failed_table;
  MDL_savepoint m_start_of_statement_svp;
  /**
    Lock timeout in nanoseconds. Initialized to LONG_TIMEOUT when opening system
    tables or to the "lock_wait_timeout" system variable for regular tables.
  */
  ulonglong m_timeout_nsec;
  /* open_table() flags. */
  uint m_flags;
  /** Back off action. */
  enum enum_open_table_action m_action;
  /**
    Whether we had any locks when this context was created.
    If we did, they are from the previous statement of a transaction,
    and we can't safely do back-off (and release them).
  */
  bool m_has_locks;
  /**
    Indicates that in the process of opening tables we have acquired
    protection against global read lock.
  */
  bool m_has_protection_against_grl;
};


/**
  Check if a TABLE_LIST instance represents a pre-opened temporary table.
*/

inline bool is_temporary_table(TABLE_LIST *tl)
{
  if (tl->view || tl->schema_table)
    return FALSE;

  if (!tl->table)
    return FALSE;

  /*
    NOTE: 'table->s' might be NULL for specially constructed TABLE
    instances. See SHOW TRIGGERS for example.
  */

  if (!tl->table->s)
    return FALSE;

  return tl->table->s->tmp_table != NO_TMP_TABLE;
}


/**
  This internal handler is used to trap ER_NO_SUCH_TABLE.
*/

class No_such_table_error_handler : public Internal_error_handler
{
public:
  No_such_table_error_handler()
    : m_handled_errors(0), m_unhandled_errors(0)
  {}

  bool handle_condition(THD *thd,
                        uint sql_errno,
                        const char* sqlstate,
                        Sql_condition::enum_warning_level level,
                        const char* msg,
                        Sql_condition ** cond_hdl);

  /**
    Returns TRUE if one or more ER_NO_SUCH_TABLE errors have been
    trapped and no other errors have been seen. FALSE otherwise.
  */
  bool safely_trapped_errors();

private:
  int m_handled_errors;
  int m_unhandled_errors;
};

#endif /* SQL_BASE_INCLUDED */
