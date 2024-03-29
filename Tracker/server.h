#if !defined(AFX_SERVER_H__B9726CD5_D101_4193_A555_69102FC058E9__INCLUDED_)
#define AFX_SERVER_H__B9726CD5_D101_4193_A555_69102FC058E9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "sql/database.h"
#include "config.h"
#include "connection.h"
#include "epoll.h"
#include "peer_link.h"
#include "stats.h"
#include "tcp_listen_socket.h"
#include "tracker_input.h"
#include "udp_listen_socket.h"
#include <functional>

const int MAX_PEERS = 50;  //maximum returned peers
const int LOCAL_IP_1_BYTE = 10; // first byte for intranet
const int BOB_INTERNAL_IP = 0x7072720a;
const int BOB_EXTERNAL_IP = 0x222071d4;
const int MINIMUM_PEERS = 3; // minimum peers when bob and other magic peers are not give


class Cserver
{
public:
	struct t_peer
	{
		t_peer()
		{
			listening = false;
			mtime = 0;
		}

		long long downloaded;
		long long left;
		string peer_id;
		int port;
		int uid;
		long long uploaded;

		bool listening;
		time_t mtime;
	};

	typedef map<pair<int, int>, t_peer> t_peers;

	class Cannounce_output
	{
	public:
		virtual void peer(int h, const t_peer&) = 0;

		void complete(int v)
		{
			m_complete = v;
		}

		void incomplete(int v)
		{
			m_incomplete = v;
		}

		void interval(int v)
		{
			m_interval = v;
		}

		Cannounce_output()
		{
			m_complete = 0;
			m_incomplete = 0;
			m_interval = 1800;
		}
	protected:
		int m_complete;
		int m_incomplete;
		int m_interval;
	};

	struct t_deny_from_host
	{
		unsigned int end;
		bool marked;
	};

	struct t_file
	{
        typedef vector<t_peers::const_iterator> t_candidates;

		void clean_up(time_t t, Cserver&);
		string debug() const;

        /// @brief make up peer list for current request
        /// @param ti - current client
        /// @param o - generating output
        void select_peers(const Ctracker_input & ti, Cannounce_output & o) const;

        /// @brief move external peers to the and of peerlist
        /// @param ti - current client
        /// @param cand - peers candidates
        /// @return - numbers of internal peers
        int move_external_peers(const Ctracker_input & ti, t_candidates & cand) const;

        /// @brief move internal peers to the and of peerlist
        /// @param ti - current client
        /// @param cand - peers candidates
        /// @return - numbers of internal peers
        int move_internal_peers(const Ctracker_input & ti, t_candidates & cand) const;

        /// @brief get N candidates (depend from ti)
        /// @param cand - peers candidates
        void crop_n_peers(t_candidates & cand, size_t n) const;

        /// @brief remove BOB and other "magic" peers
        /// @param cand - peers candidates
        void remove_bob(t_candidates & cand) const;

        bool i_am_bob(const int ip) const;

        struct is_internal_peer : unary_function<t_peers::const_iterator, bool>
        {
            bool operator ()(t_peers::const_iterator peer)const;
        };



		Cbvalue scrape() const;

		t_file()
		{
			completed = 0;
			dirty = true;
			fid = 0;
			leechers = 0;
			seeders = 0;
		}

		t_peers peers;
		int completed;
		bool dirty;
		bool freetorrent;
		int fid;
		int leechers;
		int seeders;
		time_t ctime;
		long long balance;
	};


	struct t_user
	{
		t_user()
		{
			completes = 0;
			incompletes = 0;
		}

		bool can_leech;
		bool marked;
		int uid;
		int completes;
		int incompletes;
		int peers_limit;
		int torrents_limit;
		int wait_time;
            double seedbonus;
		string pass;
		long long torrent_pass_secret;
	};

	typedef map<string, t_file> t_files;
	typedef map<unsigned int, t_deny_from_host> t_deny_from_hosts;
	typedef map<int, t_user> t_users;
	typedef map<string, t_user*> t_users_names;
	typedef map<string, t_user*> t_users_torrent_passes;

	int test_sql();
	void accept(const Csocket& l);
	t_user* find_user_by_name(const string&);
	t_user* find_user_by_torrent_pass(const string&);
	t_user* find_user_by_uid(int);
	void read_config();
	void write_db_files();
	void write_db_users();
	void read_db_deny_from_hosts();
	void read_db_files();
	void read_db_files_sql();
	void read_db_users();
	void clean_up();
	string insert_peer(const Ctracker_input&, bool listen_check, bool udp, t_user*);
	void update_peer(const string& file_id, pair<int, int> peer_id, bool listening);
	string debug(const Ctracker_input&) const;
	string statistics() const;
	Cbvalue select_peers(const Ctracker_input&, const t_user*);
	Cbvalue scrape(const Ctracker_input&);
	int run();
	static void term();
	Cserver(Cdatabase&, const string& table_prefix, bool use_sql);

	int announce_interval() const
	{
		return m_config.m_announce_interval;
	}

	const t_file* file(const string& id) const
	{
		t_files::const_iterator i = m_files.find(id);
		return i == m_files.end() ? NULL : &i->second;
	}

	bool anonymous_connect() const
	{
		return m_config.m_anonymous_connect;
	}

	bool anonymous_announce() const
	{
		return m_config.m_anonymous_announce;
	}

	bool anonymous_scrape() const
	{
		return m_config.m_anonymous_scrape;
	}

	bool debug() const
	{
		return m_config.m_debug;
	}

	bool gzip_announce() const
	{
		return m_config.m_gzip_announce;
	}

	bool gzip_debug() const
	{
		return m_config.m_gzip_debug;
	}

	bool gzip_scrape() const
	{
		return m_config.m_gzip_scrape;
	}

	const string& redirect_url() const
	{
		return m_config.m_redirect_url;
	}

	long long secret() const
	{
		return m_secret;
	}

	Cstats& stats()
	{
		return m_stats;
	}

	time_t time() const
	{
		return m_time;
	}
private:
	enum
	{
		column_files_completed,
		column_files_fid,
		column_files_leechers,
		column_files_seeders,
		column_users_uid,
		table_announce_log,
		table_config,
		table_deny_from_hosts,
		table_files,
		table_files_users,
		table_scrape_log,
		table_users,
	};

	typedef list<Cconnection> t_connections;
	typedef list<Cpeer_link> t_peer_links;
	typedef list<Ctcp_listen_socket> t_tcp_sockets;
	typedef list<Cudp_listen_socket> t_udp_sockets;

	static void sig_handler(int v);
	string column_name(int v) const;
	string table_name(int) const;

	Cconfig m_config;
	Cstats m_stats;
	bool m_use_sql;
	time_t m_clean_up_time;
	time_t m_read_config_time;
	time_t m_read_db_deny_from_hosts_time;
	time_t m_read_db_files_time;
	time_t m_read_db_users_time;
	time_t m_time;
	time_t m_write_db_files_time;
	time_t m_write_db_users_time;
	int m_fid_end;
	long long m_secret;
	t_connections m_connections;
	t_peer_links m_peer_links;
	Cdatabase& m_database;
	Cepoll m_epoll;
	t_deny_from_hosts m_deny_from_hosts;
	t_files m_files;
	t_users m_users;
	t_users_names m_users_names;
	t_users_torrent_passes m_users_torrent_passes;
	string m_announce_log_buffer;
	string m_files_users_updates_buffer;
	string m_scrape_log_buffer;
	string m_table_prefix;
	string m_users_updates_buffer;
};

#endif // !defined(AFX_SERVER_H__B9726CD5_D101_4193_A555_69102FC058E9__INCLUDED_)
