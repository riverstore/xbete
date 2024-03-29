#include "stdafx.h"
#include "bt_misc.h"

#include <sys/stat.h>
#include <ctime>
#include "socket.h"

#ifdef WIN32
#pragma comment(lib, "ws2_32")
#endif

string escape_string(const string& v)
{
	string w;
	w.reserve(v.length());
	for (size_t i = 0; i < v.length(); i++)
	{
		if (isgraph(v[i]))
			w += v[i];
		else
		{
			switch (v[i])
			{
			case '\0':
				w += "\\0";
				break;
			default:
				w += "\\x" + hex_encode(2, v[i]);
			}
		}
	}
	return w;
}

string get_env(const string& v)
{
	const char* p = getenv(v.c_str());
	return p ? p : "";
}

static int hex_decode(char v)
{
	if (v >= '0' && v <= '9')
		return v - '0';
	if (v >= 'A' && v <= 'Z')
		return v - 'A' + 10;
	if (v >= 'a' && v <= 'z')
		return v - 'a' + 10;
	return -1;
};

string hex_decode(const string& v)
{
	string r;
	r.resize(v.length() >> 1);
	for (size_t i = 0; i + 2 <= v.length(); i += 2)
	{
		int a = hex_decode(v[i]);
		r[i >> 1] = a << 4 | hex_decode(v[i + 1]);
	}
	return r;
}

string hex_encode(int l, int v)
{
	string r;
	r.resize(l);
	while (l--)
	{
		r[l] = "0123456789abcdef"[v & 0xf];
		v >>= 4;
	}
	return r;
};

string n(long long v)
{
	char b[21];
#ifdef WIN32
	sprintf(b, "%I64d", v);
#else
	sprintf(b, "%lld", v);
#endif
	return b;
}

string hex_encode(const string& v)
{
	string r;
	r.reserve(v.length() << 1);
	for (size_t i = 0; i < v.length(); i++)
		r += hex_encode(2, v[i]);
	return r;
}

string js_encode(const string& v)
{
	string r;
	for (size_t i = 0; i < v.length(); i++)
	{
		switch (v[i])
		{
		case '\"':
		case '\'':
		case '\\':
			r += '\\';
		default:
			r += v[i];
		}
	}
	return r;
}

string uri_decode(const string& v)
{
	string r;
	r.reserve(v.length());
	for (size_t i = 0; i < v.length(); i++)
	{
		char c = v[i];
		switch (c)
		{
		case '%':
			{
				if (i + 1 > v.length())
					return "";
				int l = v[++i];
				r += hex_decode(l) << 4 | hex_decode(v[++i]);
				break;
			}
		case '+':
			r += ' ';
			break;
		default:
			r += c;
		}
	}
	return r;
};

string uri_encode(const string& v)
{
	string r;
	r.reserve(v.length());
	for (size_t i = 0; i < v.length(); i++)
	{
		char c = v[i];
		if (isalpha(c & 0xff) || isdigit(c & 0xff))
			r += c;
		else
		{
			switch (c)
			{
			case ' ':
				r += '+';
				break;
			case '-':
			case ',':
			case '.':
			case '@':
			case '_':
				r += c;
				break;
			default:
				r += "%" + hex_encode(2, c);
			}
		}
	}
	return r;
};

bool is_private_ipa(int a)
{
	return (ntohl(a) & 0xff000000) == 0x0a000000
		|| (ntohl(a) & 0xff000000) == 0x7f000000
		|| (ntohl(a) & 0xfff00000) == 0xac100000
		|| (ntohl(a) & 0xffff0000) == 0xc0a80000;
}

string b2a(long long v, const char* postfix)
{
	int l;
	for (l = 0; v < -9999 || v > 999999; l++)
		v >>= 10;
	char d[32];
	char* w = d;
	if (v > 999)
	{
		l++;
		int b = static_cast<int>((v & 0x3ff) * 100 >> 10);
		v >>= 10;
		w += sprintf(w, "%d", static_cast<int>(v));
		if (v < 10 && b % 10)
			w += sprintf(w, ".%02d", b);
		else if (v < 100 && b > 9)
			w += sprintf(w, ".%d", b / 10);
	}
	else
		w += sprintf(w, "%d", static_cast<int>(v));
	const char* a[] = {"", " k", " m", " g", " t", " p", " e", " z", " y"};
	w += sprintf(w, "%s", a[l]);
	if (postfix)
		w += sprintf(w, "%s%s", l ? "" : " ", postfix);
	return d;
}

static string peer_id2a(const string& name, const string& peer_id, int i)
{
	for (size_t j = i; j < peer_id.size(); j++)
	{
		if (!isalnum(peer_id[j]))
			return name + peer_id.substr(i, j - i);
	}
	return name + peer_id.substr(i);
}

string peer_id2a(const string& v)
{
	if (v.length() != 20)
		return "";
	if (v[7] == '-')
	{
		switch (v[0])
		{
		case '-':
			if (v[1] == 'A' && v[2] == 'Z')
				return peer_id2a("Azureus ", v, 3);
			if (v[1] == 'B' && v[2] == 'C')
				return peer_id2a("BitComet ", v, 3);
			if (v[1] == 'U' && v[2] == 'T')
				return peer_id2a("uTorrent ", v, 3);
			if (v[1] == 'T' && v[2] == 'S')
				return peer_id2a("TorrentStorm ", v, 3);
			break;
		case 'A':
			return peer_id2a("ABC ", v, 1);
		case 'M':
			return peer_id2a("Mainline ", v, 1);
		case 'S':
			return peer_id2a("Shadow ", v, 1);
		case 'T':
			return peer_id2a("BitTornado ", v, 1);
		case 'X':
			if (v[1] == 'B' && v[2] == 'T')
				return peer_id2a("XBT Client ", v, 3) + (v.find_first_not_of("0123456789ABCDEFGHIJKLMNOPQRSTUVWYXZabcdefghijklmnopqrstuvwyxz", 8) == string::npos ? "" : " (fake)");
			break;
		}
	}
	switch (v[0])
	{
	case '-':
		if (v[1] == 'G' && v[2] == '3')
			return "G3";
		break;
	case 'S':
		if (v[1] == 5 && v[2] == 7 && v[3] >= 0 && v[3] < 10)
			return "Shadow 57" + n(v[3]);
		break;
	case 'e':
		if (v[1] == 'x' && v[2] == 'b' && v[3] == 'c' && v[4] >= 0 && v[4] < 10 && v[5] >= 0 && v[5] < 100)
			return "BitComet " + n(v[4]) + '.' + n(v[5] / 10) + n(v[5] % 10);
	}
	return "Unknown";
}

string duration2a(float v)
{
	char d[32];
	if (v > 86400)
		sprintf(d, "%.1f days", v / 86400);
	else if (v > 3600)
		sprintf(d, "%.1f hours", v / 3600);
	else if (v > 60)
		sprintf(d, "%.1f minutes", v / 60);
	else
		sprintf(d, "%.1f seconds", v);
	return d;
}

string time2a(time_t v)
{
	const tm* date = localtime(&v);
	if (!date)
		return "";
	char b[20];
	sprintf(b, "%04d-%02d-%02d %02d:%02d:%02d", date->tm_year + 1900, date->tm_mon + 1, date->tm_mday, date->tm_hour, date->tm_min, date->tm_sec);
	return b;
}

int merkle_tree_size(int v)
{
	int r = 0;
	while (v > 1)
	{
		r += v++;
		v >>= 1;
	}
	if (v == 1)
		r++;
	return r;
}

string backward_slashes(string v)
{
	for (size_t i = 0; (i = v.find('/', i)) != string::npos; i++)
		v[i] = '\\';
	return v;
}

string forward_slashes(string v)
{
	for (size_t i = 0; (i = v.find('\\', i)) != string::npos; i++)
		v[i] = '/';
	return v;
}

string native_slashes(const string& v)
{
#ifdef WIN32
	return backward_slashes(v);
#else
	return forward_slashes(v);
#endif
}

int mkpath(const string& v)
{
	for (size_t i = 0; i < v.size(); )
	{
		size_t a = v.find_first_of("/\\", i);
		if (a == string::npos)
			a = v.size();
#ifdef WIN32
		CreateDirectory(v.substr(0, a).c_str(), NULL);
#else
		mkdir(v.substr(0, a).c_str(), 0777);
#endif
		i = a + 1;
	}
	return 0;
}

int hms2i(int h, int m, int s)
{
	return 60 * (h + 60 * m) + s;
}

string xbt_version2a(int v)
{
	return n(v / 100) + "." + n(v / 10 % 10) + "." + n(v % 10);
}

#ifdef BSD
long long xbt_atoll(const char* v)
{
	return strtoll(v, NULL, 10);
}
#endif
