#include "readfbxprim.h"
#include "util/log.h"
#include <zenomodel/include/api.h>
#include <zenomodel/include/igraphsmodel.h>
#include "zenoapplication.h"
#include <zenomodel/include/graphsmanagment.h>

#include <viewport/zenovis.h>
#include <zeno/core/Session.h>
#include <zenovis/ObjectsManager.h>
#include <zeno/extra/GlobalState.h>
#include <zeno/types/UserData.h>
#include <zeno/extra/TempNode.h>
#include <zeno/types/StringObject.h>

#include <filesystem>
#include <vector>

int my_sqrt(int x) {
    // Base case
    if (x == 0 || x == 1) {
        return x;
    }

    // Initialize variables
    int y = x;
    int z = 1;

    // Apply Babylonian method until convergence
    while (y > z) {
        y = (y + z) / 2;
        z = x / y;
    }

    // Return the floor of the final square root
    return y;
}

using Path = std::filesystem::path;

ReadFBXPrim::ReadFBXPrim(const NodeUtilParam& params, QGraphicsItem* parent)
    : ZenoNode(params, parent)
{

}

ReadFBXPrim::~ReadFBXPrim()
{

}

ZGraphicsLayout* ReadFBXPrim::initCustomParamWidgets()
{
    ZGraphicsLayout* pHLayout = new ZGraphicsLayout(true);

    ZSimpleTextItem* pNameItem = new ZSimpleTextItem("node");
    pNameItem->setBrush(m_renderParams.socketClr.color());
    pNameItem->setFont(m_renderParams.socketFont);
    pNameItem->updateBoundingRect();
    pHLayout->addItem(pNameItem);

    ZenoParamPushButton* pEditBtn = new ZenoParamPushButton("Generate", -1, QSizePolicy::Expanding);
    pHLayout->addItem(pEditBtn);
    connect(pEditBtn, SIGNAL(clicked()), this, SLOT(onEditClicked()));

    _param_ctrl param;
    param.param_name = pNameItem;
    param.param_control = pEditBtn;
    param.ctrl_layout = pHLayout;
    addParam(param);

    return pHLayout;
}

void ReadFBXPrim::onEditClicked()
{
    zeno::log_info("ReadFBXPrim Generate Nodes");
    ZENO_HANDLE hGraph = Zeno_GetGraph("main");

    // Get ReadFBXPrim ident
    ZENO_HANDLE fbxNode = index().internalId();
    std::string fbxIdentStr = index().data(ROLE_OBJID).toString().toStdString();

    // Get FBX Path
    ZVARIANT path; std::string type;
    Zeno_GetInputDefl(hGraph, fbxNode, "path", path, type);

    // Get FBX HintPath
    ZVARIANT hintPath;
    Zeno_GetInputDefl(hGraph, fbxNode, "hintPath", hintPath, type);

    std::string get_path = std::get<std::string>(path);
    std::string get_hintPath = std::get<std::string>(hintPath);

    auto path_ = std::make_shared<zeno::StringObject>(); path_->set(get_path);
    auto hintPath_ = std::make_shared<zeno::StringObject>(); hintPath_->set(get_hintPath);

    auto fbxFileName = Path(get_path).replace_extension("").filename().string();

    // Get basic information in FBX
    zeno::log_info("ReadFBXPrim TempNodeSimpleCaller FBX Name {}", fbxFileName);
    auto outs = zeno::TempNodeSimpleCaller("ReadFBXPrim")
        .set("path", path_)
        .set("hintPath", hintPath_)
        .set2<bool>("generate", true)
        .set2<std::string>("udim:", "DISABLE")
        .set2<bool>("invOpacity:", true)
        .set2<bool>("primitive:", false)
        .set2<bool>("printTree:", false)
        .set2<bool>("triangulate:", true)
        .call();
    zeno::log_info("ReadFBXPrim Caller End");

    // Get the position of current ReadFBXPrim node
    std::pair<float, float> fbxNodePos;
    Zeno_GetPos(hGraph, fbxNode, fbxNodePos);

    // Create nodes
    auto fbxObj = outs.get("prim");

    if(fbxObj) {
        auto matNum = fbxObj->userData().getLiterial<int>("matNum");
        auto fbxName = fbxObj->userData().getLiterial<std::string>("fbxName");

        ZENO_HANDLE fbxPartGraph = Zeno_GetGraph("FBXPart");
        ZASSERT_EXIT(fbxPartGraph);

        int my_x = my_sqrt(matNum);
        int my_i = 0;
        float add_x_pos = 0.0f;
        float add_y_pos = 0.0f;

        for (int i = 0; i < matNum; i++) {
            auto matName = fbxObj->userData().getLiterial<std::string>(std::to_string(i));
            zeno::log_info("Create with mat name {}, fbx name {}", matName, fbxName);

            std::cout<<"total:"<<matNum<<", current:"<<i<<"\n";

            ZENO_HANDLE dictNode = Zeno_AddNode(hGraph, "DictGetItem");
            add_y_pos = my_i * 300.0f;
            my_i++;
            if(i % my_x == 0){
                add_x_pos += 1500.0f;
                my_i = 0;
                add_y_pos = 0.0f;
            }

            std::pair<float, float> dictNodePos = {fbxNodePos.first + 600.0f + add_x_pos, fbxNodePos.second + add_y_pos};
            Zeno_SetPos(hGraph,dictNode, dictNodePos);

            Zeno_SetInputDefl(hGraph, dictNode, "key", matName);
            Zeno_AddLink(hGraph, fbxNode, "mats", dictNode, "dict");

            ZENO_HANDLE forkedSubg = 0;
            ZENO_HANDLE forkedNode = 0;
            std::string concatname = matName;
            if (matName.find(':') != std::string::npos) {
                std::replace(concatname.begin(), concatname.end(), ':', '_');
            }
            std::string fbxPartGraphName = fbxName+"_"+concatname;
            ZENO_ERROR ret = Zeno_ForkGraph(hGraph, "FBXPart", forkedSubg, forkedNode);
            ZASSERT_EXIT(!ret);ZASSERT_EXIT(forkedSubg);ZASSERT_EXIT(forkedNode);

            // Add Texture2D node
//            ZENO_HANDLE listNode1 = Zeno_AddNode(forkedSubg, "MakeSmallList");
//            ZENO_HANDLE listNode2 = Zeno_AddNode(forkedSubg, "MakeSmallList");
//            ZENO_HANDLE listNode3 = Zeno_AddNode(forkedSubg, "MakeSmallList");
//            float yoff = 3000.0f;
//            Zeno_SetPos(forkedSubg, listNode1, {-6000.0f, yoff + 0.0f});
//            Zeno_SetPos(forkedSubg, listNode2, {-6000.0f, yoff + 200.0f});
//            Zeno_SetPos(forkedSubg, listNode3, {-6000.0f, yoff + 400.0f});
//            std::vector<ZENO_HANDLE> tex2dnodes;

//            for(int j=0;j<15;j++){
//                ZENO_HANDLE tmpTex2dNode = Zeno_AddNode(forkedSubg, "MakeTexture2D");
//                std::pair<float, float> tex2dnodePos = {-7000.0f, yoff+j*200.0f};
//                Zeno_SetPos(forkedSubg, tmpTex2dNode, tex2dnodePos);
//                tex2dnodes.push_back(tmpTex2dNode);
//
//                auto texPath = fbxObj->userData().getLiterial<std::string>(
//                    std::to_string(i)+"_tex_"+std::to_string(j));
//                Zeno_SetInputDefl(forkedSubg, tmpTex2dNode, "path", texPath);
//
//                if(j<6)
//                    Zeno_AddLink(forkedSubg, tmpTex2dNode, "tex", listNode1, "obj"+std::to_string(j%6));
//                else if(j<12)
//                    Zeno_AddLink(forkedSubg, tmpTex2dNode, "tex", listNode2, "obj"+std::to_string(j%6));
//                else
//                    Zeno_AddLink(forkedSubg, tmpTex2dNode, "tex", listNode3, "obj"+std::to_string(j%6));
//            }
//
//            ZENO_HANDLE extendList1 = Zeno_AddNode(forkedSubg, "ExtendList");
//            ZENO_HANDLE extendList2 = Zeno_AddNode(forkedSubg, "ExtendList");
//            Zeno_SetPos(forkedSubg, extendList1, {-5000.0f, yoff + 0.0f});
//            Zeno_SetPos(forkedSubg, extendList2, {-5000.0f, yoff + 200.0f});
//            Zeno_AddLink(forkedSubg, listNode1, "list", extendList1, "list1");
//            Zeno_AddLink(forkedSubg, listNode2, "list", extendList1, "list2");
//            Zeno_AddLink(forkedSubg, extendList1, "list1", extendList2, "list1");
//            Zeno_AddLink(forkedSubg, listNode3, "list", extendList2, "list2");
//
//            ZENO_HANDLE texListsPortal = Zeno_AddNode(forkedSubg, "PortalIn");
//            Zeno_SetPos(forkedSubg, texListsPortal, {-4000.0f, yoff + 0.0f});
//            std::string stexLists("texLists");
//            Zeno_SetParam(forkedSubg, texListsPortal, "name", stexLists);
//            Zeno_AddLink(forkedSubg, extendList2, "list1", texListsPortal, "port");

            Zeno_RenameGraph(forkedSubg, fbxPartGraphName);

            std::pair<float, float> fbxPartPos = {dictNodePos.first + 500.0f, dictNodePos.second};

            Zeno_SetPos(hGraph, forkedNode, fbxPartPos);
            Zeno_AddLink(hGraph, dictNode, "object", forkedNode, "data");
            Zeno_SetView(hGraph, forkedNode, true);
        }


        Zeno_SetInputDefl(hGraph, fbxNode, "generate", false);
    }else{
        zeno::log_error("Not found ReadFBXPrim node in objectsMan");
    }
}
