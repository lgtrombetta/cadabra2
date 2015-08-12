
#include "DataCell.hh"
#include <sstream>

using namespace cadabra;

uint64_t DataCell::max_serial_number=0;
std::mutex DataCell::serial_mutex;

DataCell::DataCell(CellType t, const std::string& str, bool texhidden) 
	{
	cell_type = t;
	textbuf = str;
	tex_hidden = texhidden;
	running=false;
	
	std::lock_guard<std::mutex> guard(serial_mutex);
	serial_number = max_serial_number++;
	}

DataCell::DataCell(const DataCell& other)
	{
	cell_type = other.cell_type;
	textbuf = other.textbuf;
	cdbbuf = other.cdbbuf;
	tex_hidden = other.tex_hidden;
	sensitive = other.sensitive;
	serial_number = other.serial_number;
	}

std::string cadabra::JSON_serialise(const DTree& doc)
	{
	Json::Value json;
	JSON_recurse(doc, doc.begin(), json);

	std::ostringstream str;
	str << json;

	return str.str();
	}

void cadabra::JSON_recurse(const DTree& doc, DTree::iterator it, Json::Value& json)
	{
	switch(it->cell_type) {
		case DataCell::CellType::document:
			json["description"]="Cadabra JSON notebook format";
			json["version"]=1.0;
			break;
		case DataCell::CellType::input:
			json["cell_type"]="input";
			break;
		case DataCell::CellType::output:
			json["cell_type"]="output";
			break;
//		case DataCell::CellType::comment:
//			json["cell_type"]="comment";
//			break;
//		case DataCell::CellType::texcomment:
//			json["cell_type"]="texcomment";
//			break;
		case DataCell::CellType::latex:
			json["cell_type"]="tex";
			break;
		case DataCell::CellType::error:
			json["cell_type"]="error";
			break;
//		case DataCell::CellType::section: {
//			assert(1==0);
//			// NOT YET FUNCTIONAL
//			json["cell_type"]="section";
//			Json::Value child;
//			child["content"]="test";
//			json.append(child);
//			break;
//			}
		}
	if(it->cell_type!=DataCell::CellType::document) {
		json["textbuf"]  =it->textbuf;
		json["id"]       =(Json::UInt64)it->id();
		}

	if(doc.number_of_children(it)>0) {
		Json::Value cells;
		DTree::sibling_iterator sib=doc.begin(it);
		while(sib!=doc.end(it)) {
			Json::Value thiscell;
			JSON_recurse(doc, sib, thiscell);
			cells.append(thiscell);
			++sib;
			}
		json["cells"]=cells;
		}
	}

void cadabra::JSON_deserialise(const std::string& cj, DTree& doc) 
	{
	Json::Reader reader;
	Json::Value  root;

	bool ret = reader.parse( cj, root );
	if ( !ret ) {
		std::cerr << "cannot parse json file" << std::endl;
		return;
		}

	// Setup main document.
	DataCell top(DataCell::CellType::document);
	DTree::iterator doc_it = doc.set_head(top);

	// Scan through json file.
	const Json::Value cells = root["cells"];
	JSON_in_recurse(doc, doc_it, cells);
	}

void cadabra::JSON_in_recurse(DTree& doc, DTree::iterator loc, const Json::Value& cells)
	{
	for(unsigned int c=0; c<cells.size(); ++c) {
		const Json::Value celltype = cells[c]["cell_type"];
		const Json::Value id       = cells[c]["id"];
		const Json::Value textbuf  = cells[c]["textbuf"];
		DTree::iterator last=doc.end();
		if(celltype.asString()=="input") {
			DataCell dc(cadabra::DataCell::CellType::input, textbuf.asString(), false);
			last=doc.append_child(loc, dc);
			}
		else if(celltype.asString()=="output") {
			DataCell dc(cadabra::DataCell::CellType::output, textbuf.asString(), false);
			last=doc.append_child(loc, dc);
			}
		else if(celltype.asString()=="latex") {
			DataCell dc(cadabra::DataCell::CellType::latex, textbuf.asString(), false);
			last=doc.append_child(loc, dc);
			}
		if(last!=doc.end()) {
			if(cells[c].isMember("cells")) {
				const Json::Value subcells = cells[c]["cells"];
				JSON_in_recurse(doc, last, subcells);
				}
			}
		}
	}

uint64_t DataCell::id() const
	{
	return serial_number;
	}
