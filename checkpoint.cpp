/*
 * checkpoint.cpp
 *
 *  Created on: Jun 12, 2014
 *      Author: minh
 */

#include "checkpoint.h"
#include "tools.h"
#include "timeutil.h"
#include "gzstream.h"


Checkpoint::Checkpoint() {
	filename = "";
    prev_dump_time = 0;
    dump_interval = 30; // dumping at most once per 30 seconds
    struct_name = "";
}


Checkpoint::~Checkpoint() {
}


const char* CKP_HEADER = "--- # IQ-TREE Checkpoint";

void Checkpoint::setFileName(string filename) {
	this->filename = filename;
}
void Checkpoint::load() {
	assert(filename != "");
    if (!fileExists(filename)) return;
    try {
        igzstream in;
        // set the failbit and badbit
        in.exceptions(ios::failbit | ios::badbit);
        in.open(filename.c_str());
        // remove the failbit
        in.exceptions(ios::badbit);
        string line;
        if (!getline(in, line)) {
            in.close();
            return;
        }
        if (line != CKP_HEADER)
        	throw ("Invalid checkpoint file " + filename);
        string struct_name;
        size_t pos;
        int listid = 0;
        while (!in.eof()) {
        	getline(in, line);
            pos = line.find('#');
            if (pos != string::npos)
                line.erase(pos);
            line.erase(line.find_last_not_of("\n\r\t")+1);
//            trimString(line);
            if (line.empty()) continue;
            if (line[0] != ' ') {
                struct_name = "";
            }
//            trimString(line);
            line.erase(0, line.find_first_not_of(" \n\r\t"));
            if (line.empty()) continue;
        	pos = line.find(": ");
        	if (pos != string::npos) {
                // mapping
                (*this)[struct_name + line.substr(0, pos)] = line.substr(pos+2);
            } else if (line[line.length()-1] == ':') {
                // start a new struct
                line.erase(line.length()-1);
                trimString(line);
                struct_name = line + '.';
                listid = 0;
                continue;
            } else {
                // collection
                (*this)[struct_name + convertIntToString(listid)] = line;
                listid++;
//        		throw "':' is expected between key and value";
            }
        }
        in.clear();
        // set the failbit again
        in.exceptions(ios::failbit | ios::badbit);
        in.close();
    } catch (ios::failure &) {
        outError(ERR_READ_INPUT);
    } catch (const char *str) {
        outError(str);
    } catch (string &str) {
        outError(str);
    }
}

void Checkpoint::setDumpInterval(double interval) {
    dump_interval = interval;
}


void Checkpoint::dump(bool force) {
	assert(filename != "");
    if (!force && getRealTime() < prev_dump_time + dump_interval) {
        return;
    }
    prev_dump_time = getRealTime();
    try {
        ogzstream out;
        out.exceptions(ios::failbit | ios::badbit);
        out.open(filename.c_str());
        out << CKP_HEADER << endl;
        string struct_name;
        size_t pos;
        int listid = 0;
        for (iterator i = begin(); i != end(); i++) {
            if ((pos = i->first.find('.')) != string::npos) {
                if (struct_name != i->first.substr(0, pos)) {
                    struct_name = i->first.substr(0, pos);
                    out << struct_name << ":" << endl;
                    listid = 0;
                }
                // check if key is a collection
                out << "  " << i->first.substr(pos+1) << ": " << i->second << endl;
            } else
                out << i->first << ": " << i->second << endl;
        }
        out.close();
//        cout << "Checkpoint dumped" << endl;
    } catch (ios::failure &) {
        outError(ERR_WRITE_OUTPUT, filename.c_str());
    }
}

bool Checkpoint::hasKey(string key) {
	return (find(key) != end());
}

/*-------------------------------------------------------------
 * series of get function to get value of a key
 *-------------------------------------------------------------*/

bool Checkpoint::getBool(string key, bool &ret) {
    string value;
    if (!get(key, value)) return false;
	if (value == "true") 
        ret = true;
    else if (value == "false") 
        ret = false;
    else
        outError("Invalid boolean value " + value + " for key " + key);
    return true;
}

bool Checkpoint::getBool(string key) {
    bool ret;
    if (!getBool(key, ret))
        return false;
    return ret;
}

/*-------------------------------------------------------------
 * series of put function to put pair of (key,value)
 *-------------------------------------------------------------*/

void Checkpoint::putBool(string key, bool value) {
    if (value)
        put(key, "true");
    else
        put(key, "false");
}


/*-------------------------------------------------------------
 * nested structures
 *-------------------------------------------------------------*/
void Checkpoint::startStruct(string name) {
    struct_name = struct_name + name + '.';
}

/**
    end the current struct
*/
void Checkpoint::endStruct() {
    size_t pos = struct_name.find_last_of('.', struct_name.length()-2);
    if (pos == string::npos)
        struct_name = "";
    else
        struct_name.erase(pos+1);
}

void Checkpoint::startList(int nelem) {
    list_element.push_back(-1);
    if (nelem > 0)
        list_element_precision.push_back((int)ceil(log10(nelem)));
    else
        list_element_precision.push_back(0);
}

void Checkpoint::addListElement() {
    list_element.back()++;
    if (list_element.back() > 0) {
        size_t pos = struct_name.find_last_of('.', struct_name.length()-2);
        assert(pos != string::npos);
        struct_name.erase(pos+1);
    }
    stringstream ss;
    ss << setw(list_element_precision.back()) << setfill('0') << list_element.back();
//    ss << list_element.back();
    struct_name += ss.str() + ".";
}

void Checkpoint::endList() {
    assert(!list_element.empty());

    if (list_element.back() >= 0) {
        size_t pos = struct_name.find_last_of('.', struct_name.length()-2);
        assert(pos != string::npos);
        struct_name.erase(pos+1);
    }

    list_element.pop_back();
    list_element_precision.pop_back();

}

void Checkpoint::getSubCheckpoint(Checkpoint *target, string partial_key) {
    for (iterator it = begin(); it != end(); it++) {
        if (it->first.find(partial_key) != string::npos)
            (*target)[it->first] = it->second;
    }
}


/*-------------------------------------------------------------
 * CheckpointFactory
 *-------------------------------------------------------------*/

CheckpointFactory::CheckpointFactory() {
    checkpoint = NULL;
}

void CheckpointFactory::setCheckpoint(Checkpoint *checkpoint) {
    this->checkpoint = checkpoint;
}

Checkpoint *CheckpointFactory::getCheckpoint() {
    return checkpoint;
}

void CheckpointFactory::saveCheckpoint() {
    // do nothing
}

void CheckpointFactory::restoreCheckpoint() {
    // do nothing
}

