#include"Meshes.h"
#include"Textures.h"
#include"CameraController.h"
#include"Bobject_Engine.h"
#include"UIElements.h"

struct ProjectorPositionObject {
	alignas(16) glm::vec4 pos = glm::vec4(0.0f);
};

class ProjectorObjPicker : public Widget {
public:
	ProjectorObjPicker(LoadList* assets, LoadList* texLL) {
		loadList = assets;
		textureLL = texLL;
	}

	void setup(std::string tName, std::vector<std::string> objects, std::function<void(UIItem*)> exitFunc, std::function<void(UIItem*)> cancelFunc) {
		if (isSetup) {
			return;
		}

		textureName = tName;
		finishCallback = exitFunc;

		std::vector<std::string> textures{};
		textureLL->listTextures(textures);

		auto it = find(textures.begin(), textures.end(), textureName);
		if (it != textures.end()) {
			textures.erase(it);
		}

		Material* invisibleMat = loadList->getMaterial("UnrenderedBtnMat");
		Material* visibleMat = loadList->getMaterial("TestCheckBtnMat");
		Material* finishMat = loadList->getMaterial("FinishBtnMat");
		Material* cancelMat = loadList->getMaterial("CancelBtnMat");

		Arrangement* totalArrangement = new Arrangement(ORIENT_VERTICAL, 0.0f, 0.0f, 0.25f, 0.8f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);

		TextBox* topText = new TextBox(loadList->getFont(), 0.0f, 0.0f, 5.0f, 1.0f, 18, ARRANGE_START, ARRANGE_CENTER);
		topText->addText("Projecting " + textureName + " onto Object:");

		totalArrangement->addItem(getPtr(topText));

		DropdownMenu* objSelect = new DropdownMenu(0.0f, 0.0f, 5.0f, 1.0f, invisibleMat, visibleMat, loadList->getMaterial("UIRoundBox"), loadList->getFont());
		objSelect->setBlankText("Select an object");
		objSelect->addOptions(objects);
		objSelect->setSelectCallback(std::bind(&ProjectorObjPicker::setObject, this, std::placeholders::_1));

		totalArrangement->addItem(getPtr(objSelect));

		Arrangement* finishArrangement = new Arrangement(ORIENT_HORIZONTAL, 0.0f, 0.0f, 5.0f, 1.0f, 0.01f, ARRANGE_START, SCALE_BY_DIMENSIONS);
		Button* cancelBtn = new Button(cancelMat, cancelFunc);
		cancelBtn->Name = tName;

		finishArrangement->addItem(getPtr(cancelBtn));
		finishArrangement->addItem(getPtr(new spacer()));
		finishArrangement->addItem(getPtr(new Button(finishMat, std::bind(&ProjectorObjPicker::finishFunc, this, std::placeholders::_1))));

		totalArrangement->addItem(getPtr(finishArrangement));
		totalArrangement->arrangeItems();

		canvas.push_back(getPtr(totalArrangement));

		addBackground(loadList->getMaterial("UIRoundBox"));

		isSetup = true;
	}
private:
	LoadList* textureLL = nullptr;
	std::string objectName = "";

	std::string textureName = "";
	std::function<Mesh* (std::string)> getMesh = nullptr;
	std::function<void(UIItem*)> finishCallback = nullptr;

	void setObject(UIItem* owner) {
		objectName = owner->text;
	}

	void finishFunc(UIItem* owner) {
		if (objectName == "") {
			if (errorFunc != nullptr) {
				errorFunc("SELECT ERROR: ", "No base object has been selected!");
			}
			return;
		}
		owner->Name = textureName;
		owner->text = objectName;
		if (finishCallback != nullptr) {
			finishCallback(owner);
		}
	}
};

class Projector {
public:
	Projector(Texture* t, Mesh* m, std::string tN, std::function<void(std::string, Texture*)> saveFnc, GraphicsPass* gP) {
		tex = t;
		refMesh = m;
		for (uint32_t i = 0; i != refMesh->vertices.size(); i++) {
			originalUVs.push_back(refMesh->vertices[i].texCoord);
		}
		saveTextureFnc = saveFnc;
		texName = tN;
		
		createPositionBuffer();
		updatePositionBuffer();
		
		std::vector<std::pair<std::vector<VkBuffer>, uint32_t>> projectorBuffers{std::pair<std::vector<VkBuffer>, uint32_t> {Engine::get()->uniformBuffers, sizeof(UniformBufferObject)}, std::pair<std::vector<VkBuffer>, uint32_t>{std::vector<VkBuffer>{positionBuffer}, sizeof(ProjectorPositionObject)} };
		
		shadedProjection = new Material(std::vector<Texture*>{tex}, projectorBuffers, "Affine_BF", gP);
		flatProjection = new Material(std::vector<Texture*>{tex}, projectorBuffers, "Affine_Flat", gP);
	}

	void cleanup();

	std::function<void(std::string, Texture*)> saveTextureFnc = nullptr;

	void setCamera(Camera*);
	void shareCamera(Camera*);

	void calculateUVs();

	void createMarkerVertices(float, Vertex&, Vertex&, Vertex&, Vertex&);
	void makeProjectorMarker();
	void makeBackgroundMarker();

	void createPositionBuffer();
	void updatePositionBuffer();

	ProjectorPositionObject ppo{};

	VkBuffer positionBuffer = nullptr;
	void* positionBufferMapped = nullptr;
	VkDeviceMemory positionBufferMemory = nullptr;

	Texture* tex;
	std::string texName{};
	Mesh* refMesh;
	Mesh* shellMesh = nullptr;
	Mesh* reprojectionMesh = nullptr;

	Mesh* backgroundRect = nullptr;
	Mesh* projectorMarker = nullptr;

	Material* shadedProjection = nullptr;
	Material* flatProjection = nullptr;

	std::vector<glm::vec2> originalUVs{};
	std::vector<glm::vec2> projectedUVs{};

	glm::mat4 proj{};
	glm::mat4 view{};
	glm::vec3 pos{};
	glm::vec3 center{};
	float xrot = 45.0f;
	float yrot = 10.0f;
	float distance = 5.0f;
	float fov = 0.0f;

	void reprojectImage();
	void createReprojectionMesh();

private:
	struct OverlayMap {
		VkFramebuffer frameBuffer = nullptr;
		VkRenderPass renderPass = nullptr;
		Texture* colour = nullptr;
		VkDescriptorPool descriptorPool = nullptr;
		VkDescriptorSet descriptorSet = nullptr;
		VkDescriptorSetLayout descriptorSetLayout = nullptr;
		VkPipelineLayout pipelineLayout = nullptr;
		VkPipeline pipeline = nullptr;

		void cleanup() {
			vkDeviceWaitIdle(Engine::get()->device);
			if (colour != nullptr) {
				colour->cleanup();
				delete colour;
			}
			if (descriptorSetLayout != nullptr) {
				vkDestroyDescriptorSetLayout(Engine::get()->device, descriptorSetLayout, nullptr);
				vkDestroyDescriptorPool(Engine::get()->device, descriptorPool, nullptr);
			}
			if (pipeline != nullptr) {
				vkDestroyPipeline(Engine::get()->device, pipeline, nullptr);
				vkDestroyPipelineLayout(Engine::get()->device, pipelineLayout, nullptr);
			}
			if (renderPass != nullptr) {
				vkDestroyRenderPass(Engine::get()->device, renderPass, nullptr);
			}
			if (frameBuffer != nullptr) {
				vkDestroyFramebuffer(Engine::get()->device, frameBuffer, nullptr);
			}
			cleaned = true;
		}

		bool cleaned = true;
	};

	void prepMap(OverlayMap*);
	void prepareColourDescriptor(OverlayMap*);
	void createTexWritePipeline(OverlayMap*);

	VkCommandBuffer drawColourMap(VkCommandBuffer, OverlayMap*);
};