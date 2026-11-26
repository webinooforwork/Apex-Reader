#include "core/capability_registry.h"
#include <algorithm>
#include <cctype>
namespace apex {
const std::vector<Capability>& CapabilityRegistry::all() {
 static const std::vector<Capability> items{
 {"txt","Plain text",Support::Viewing,"UTF-8 text display"}, {"md","Markdown",Support::Viewing,"Displayed as source text; Markdown rendering is not yet implemented"},
 {"csv","Comma-separated values",Support::Viewing,"Text display only; no spreadsheet parsing"}, {"tsv","Tab-separated values",Support::Viewing,"Text display only"},
 {"log","Text log",Support::Viewing,"UTF-8 text display"},
 #ifdef APEX_HAS_PDF
 {"pdf","PDF",Support::Viewing,"Qt PDF (PDFium): read-only rendering, text search, page navigation, and text export"},
#else
 {"pdf","PDF",Support::Unsupported,"Qt PDF (PDFium) was not available in this build"},
#endif
#ifdef APEX_HAS_DOCX
 {"docx","Word OOXML",Support::TextExtraction,"DOCX paragraph, heading, table-cell text extraction; not page-layout rendering"},
#else
 {"docx","Word OOXML",Support::Unsupported,"DOCX engine not available in this build"},
#endif
 {"xlsx","Excel OOXML",Support::Unsupported,"No workbook parser linked yet"}, {"pptx","PowerPoint OOXML",Support::Unsupported,"No presentation parser linked yet"},
 {"odt","OpenDocument Text",Support::Unsupported,"No OpenDocument parser linked yet"}, {"ods","OpenDocument Spreadsheet",Support::Unsupported,"No OpenDocument parser linked yet"}, {"odp","OpenDocument Presentation",Support::Unsupported,"No OpenDocument parser linked yet"},
 {"rtf","Rich Text Format",Support::Unsupported,"No RTF parser linked yet"}, {"html","HTML",Support::Unsupported,"No safe HTML document engine linked yet"}, {"epub","EPUB",Support::Unsupported,"No EPUB engine linked yet"}, {"png","PNG image",Support::Unsupported,"No image viewer connected yet"}, {"jpg","JPEG image",Support::Unsupported,"No image viewer connected yet"}
 }; return items;
}
const Capability* CapabilityRegistry::find(std::string ext) { if(!ext.empty()&&ext.front()=='.') ext.erase(0,1); std::transform(ext.begin(),ext.end(),ext.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));}); const auto& v=all(); auto i=std::find_if(v.begin(),v.end(),[&](const auto& c){return c.extension==ext;}); return i==v.end()?nullptr:&*i; }
std::string CapabilityRegistry::label(Support s) { switch(s){case Support::Unsupported:return "Unsupported";case Support::Metadata:return "Metadata";case Support::TextExtraction:return "Text extraction";case Support::Viewing:return "Viewing";case Support::Editing:return "Editing";case Support::FullFidelity:return "Full fidelity";} return "Unsupported"; }
}
