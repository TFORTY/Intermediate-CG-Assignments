//Just a simple handler for simple initialization stuffs
#include "BackendHandler.h"

#include <filesystem>
#include <json.hpp>
#include <fstream>

#include <Texture2D.h>
#include <Texture2DData.h>
#include <MeshBuilder.h>
#include <MeshFactory.h>
#include <NotObjLoader.h>
#include <ObjLoader.h>
#include <VertexTypes.h>
#include <ShaderMaterial.h>
#include <RendererComponent.h>
#include <TextureCubeMap.h>
#include <TextureCubeMapData.h>

#include <Timing.h>
#include <GameObjectTag.h>
#include <InputHelpers.h>

#include <IBehaviour.h>
#include <CameraControlBehaviour.h>
#include <FollowPathBehaviour.h>
#include <SimpleMoveBehaviour.h>

int main() { 
	int frameIx = 0;
	float fpsBuffer[128];
	float minFps, maxFps, avgFps;
	int toggleMode = 0;

	BackendHandler::InitAll();

	// Let OpenGL know that we want debug output, and route it to our handler function
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(BackendHandler::GlDebugMessage, nullptr);

	// Enable texturing
	glEnable(GL_TEXTURE_2D);
	 
	// Push another scope so most memory should be freed *before* we exit the app
	{
		#pragma region Shader and ImGui
		// Load our shaders
		Shader::sptr shader = Shader::Create();
		shader->LoadShaderPartFromFile("shaders/vertex_shader.glsl", GL_VERTEX_SHADER);
		shader->LoadShaderPartFromFile("shaders/frag_blinn_phong_textured.glsl", GL_FRAGMENT_SHADER);
		shader->Link();

		glm::vec3 lightPos = glm::vec3(0.0f, 0.0f, 2.0f);
		//::vec3 lightCol = glm::vec3(0.f);//0.9,0.85,0.5
		//float     lightAmbientPow = 0.0f;//0.05
		//float     lightSpecularPow = 0.f;//1.f
		//glm::vec3 ambientCol = glm::vec3(0.0f);//1.0
		//float     ambientPow = 0.0f;//0.1 //0.38->my value
		float     lightLinearFalloff = 0.09f;
		float     lightQuadraticFalloff = 0.032f; 
		
		int		  condition = 0; 

		// These are our application / scene level uniforms that don't necessarily update
		// every frame
		shader->SetUniform("u_LightPos", lightPos); 
		//shader->SetUniform("u_LightCol", lightCol);
		//shader->SetUniform("u_AmbientLightStrength", lightAmbientPow);
		//shader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
		//shader->SetUniform("u_AmbientCol", ambientCol); 
		//shader->SetUniform("u_AmbientStrength", ambientPow);
		shader->SetUniform("u_LightAttenuationConstant", 1.0f);
		shader->SetUniform("u_LightAttenuationLinear", lightLinearFalloff); 
		shader->SetUniform("u_LightAttenuationQuadratic", lightQuadraticFalloff);

		shader->SetUniform("u_Condition", condition);
		 
		// We'll add some ImGui controls to control our shader 
		BackendHandler::imGuiCallbacks.push_back([&]() { 	 
			// No lighting toggle
			if (ImGui::Button("No Lighting"))
			{
				toggleMode = 0;
				shader->SetUniform("u_Condition", 0);
			}
			// Ambient Light Toggle
			if (ImGui::Button("Ambient Only"))
			{
				toggleMode = 1;
				shader->SetUniform("u_Condition", 1);

				shader->SetUniform("u_AmbientCol", glm::vec3(1.0f)); 
				shader->SetUniform("u_AmbientStrength", 0.38f); 
				shader->SetUniform("u_AmbientLightStrength", 0.05f); 
				shader->SetUniform("u_LightCol", glm::vec3(1.0f)); 
			}
			// Diffuse Light Toggle
			if (ImGui::Button("Diffuse Only"))
			{
				toggleMode = 2;
				shader->SetUniform("u_Condition", 2);
				 
				shader->SetUniform("u_LightCol", glm::vec3(1.0f)); 
			}   
			// Specular Light Toggle
			if (ImGui::Button("Specular Only"))
			{
				toggleMode = 3;
				shader->SetUniform("u_Condition", 3);
				   
				shader->SetUniform("u_LightCol", glm::vec3(1.0f)); 
				shader->SetUniform("u_SpecularLightStrength", 1.f);
			} 
			// Blinn-Phong Toggle
			if (ImGui::Button("Ambient + Specular + Diffuse"))
			{
				toggleMode = 4;
				shader->SetUniform("u_Condition", 4);

				shader->SetUniform("u_LightCol", glm::vec3(1.f));
				shader->SetUniform("u_AmbientLightStrength", 0.05f); 
				shader->SetUniform("u_AmbientCol", glm::vec3(1.0f)); 
				shader->SetUniform("u_AmbientStrength", 0.38f);
				shader->SetUniform("u_SpecularLightStrength", 1.f);
			}
			// Blinn-Phong w/ Toon Shading Toggle
			if (ImGui::Button("Ambient + Specular + Diffuse + Toon-Shading"))
			{
				toggleMode = 5;
				shader->SetUniform("u_Condition", 5);

				shader->SetUniform("u_LightCol", glm::vec3(1.f));
				shader->SetUniform("u_AmbientLightStrength", 0.05f);
				shader->SetUniform("u_AmbientCol", glm::vec3(1.0f));
				shader->SetUniform("u_AmbientStrength", 0.38f);
				shader->SetUniform("u_SpecularLightStrength", 1.f);
			}			
			
			ImGui::Text("Toggle Mode: ", toggleMode);

			// Displays which toggle mode is enabled on ImGui
			if (toggleMode == 0)
			{
				ImGui::SameLine(0.0f, 1.0f);
				ImGui::Text("No Lighting");
			}
			else if (toggleMode == 1)
			{
				ImGui::SameLine(0.0f, 1.0f);
				ImGui::Text("Ambient Only");
			}
			else if (toggleMode == 2)
			{
				ImGui::SameLine(0.0f, 1.0f);
				ImGui::Text("Diffuse Only");
			}
			else if (toggleMode == 3)
			{
				ImGui::SameLine(0.0f, 1.0f);
				ImGui::Text("Specular Only");
			}
			else if (toggleMode == 4)
			{
				ImGui::SameLine(0.0f, 1.0f);
				ImGui::Text("Ambient + Diffuse + Specular");
			}
			else if (toggleMode == 5)
			{
				ImGui::SameLine(0.0f, 1.0f);
				ImGui::Text("Ambient + Diffuse + Specular + Toon Shading");
			}

			minFps = FLT_MAX;
			maxFps = 0;
			avgFps = 0;
			for (int ix = 0; ix < 128; ix++) {
				if (fpsBuffer[ix] < minFps) { minFps = fpsBuffer[ix]; }
				if (fpsBuffer[ix] > maxFps) { maxFps = fpsBuffer[ix]; }
				avgFps += fpsBuffer[ix];
			}
			ImGui::PlotLines("FPS", fpsBuffer, 128);
			ImGui::Text("MIN: %f MAX: %f AVG: %f", minFps, maxFps, avgFps / 128.0f);
			});
		#pragma endregion 

		// GL states
		glEnable(GL_DEPTH_TEST);
		//glEnable(GL_CULL_FACE);
		glDepthFunc(GL_LEQUAL); // New  

		#pragma region TEXTURE LOADING
		// Load some textures from files
		Texture2D::sptr groundTex = Texture2D::LoadFromFile("images/grass.jpg");
		Texture2D::sptr chickenTex = Texture2D::LoadFromFile("images/DrumstickTexture.png");
		Texture2D::sptr stoneTex = Texture2D::LoadFromFile("images/Stone_001_Specular.png");
		Texture2D::sptr buttonTex = Texture2D::LoadFromFile("images/ButtonTexture.png");

		// Load the cube map
		TextureCubeMap::sptr environmentMap = TextureCubeMap::LoadFromImages("images/cubemaps/skybox/ocean.jpg");
		
		// Creating an empty texture
		Texture2DDescription desc = Texture2DDescription();
		desc.Width = 1;
		desc.Height = 1;
		desc.Format = InternalFormat::RGB8;
		Texture2D::sptr texture2 = Texture2D::Create(desc);
		// Clear it with a white colour
		texture2->Clear();
		#pragma endregion

		///////////////////////////////////// Scene Generation //////////////////////////////////////////////////
		#pragma region Scene Generation
		// We need to tell our scene system what extra component types we want to support
		GameScene::RegisterComponentType<RendererComponent>();
		GameScene::RegisterComponentType<BehaviourBinding>();
		GameScene::RegisterComponentType<Camera>();

		// Create a scene, and set it to be the active scene in the application
		GameScene::sptr scene = GameScene::Create("test");
		Application::Instance().ActiveScene = scene;

		// We can create a group ahead of time to make iterating on the group faster
		entt::basic_group<entt::entity, entt::exclude_t<>, entt::get_t<Transform>, RendererComponent> renderGroup =
			scene->Registry().group<RendererComponent>(entt::get_t<Transform>());

		// Create a material and set some properties for it
		ShaderMaterial::sptr material0 = ShaderMaterial::Create();
		material0->Shader = shader;
		material0->Set("s_Diffuse", groundTex);
		material0->Set("u_Shininess", 8.0f);

		ShaderMaterial::sptr material1 = ShaderMaterial::Create();
		material1->Shader = shader;
		material1->Set("s_Diffuse", chickenTex);
		material1->Set("u_Shininess", 8.0f);

		ShaderMaterial::sptr material2 = ShaderMaterial::Create();
		material2->Shader = shader;
		material2->Set("s_Diffuse", stoneTex);
		material2->Set("u_Shininess", 8.0f);

		ShaderMaterial::sptr material3 = ShaderMaterial::Create();
		material3->Shader = shader;
		material3->Set("s_Diffuse", buttonTex);
		material3->Set("u_Shininess", 8.0f);

		#pragma region Game Objects
		GameObject ground = scene->CreateEntity("ground_object");
		{
			MeshBuilder<VertexPosNormTexCol> builder = MeshBuilder<VertexPosNormTexCol>();
			MeshFactory::AddPlane(builder, glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(25.0f, 25.f), glm::vec4(1.0f));
			VertexArrayObject::sptr vao = builder.Bake();

			ground.emplace<RendererComponent>().SetMesh(vao).SetMaterial(material0);
			ground.get<Transform>().SetLocalPosition(0.0f, 0.0f, 0.0f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(ground);
		}

		// Giant chicken beind button
		GameObject chicken = scene->CreateEntity("chicken_main_object");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/ChickenStill.obj");
			chicken.emplace<RendererComponent>().SetMesh(vao).SetMaterial(material1);
			chicken.get<Transform>().SetLocalPosition(0.0f, -9.0f, 0.0f);
			chicken.get<Transform>().SetLocalRotation(glm::vec3(90.f, 0.f, 180.f));
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(chicken);
		}

		// Moving chicken left side
		GameObject chicken1 = scene->CreateEntity("chicken_object_1");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Chicken1.obj");
			chicken1.emplace<RendererComponent>().SetMesh(vao).SetMaterial(material1);
			chicken1.get<Transform>().SetLocalPosition(6.0f, -4.0f, 0.0f);
			chicken1.get<Transform>().SetLocalRotation(glm::vec3(90.f, 0.f, 180.f));
			chicken1.get<Transform>().SetLocalScale(glm::vec3(0.4f));

			// Bind returns a smart pointer to the behaviour that was added
			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(chicken1);
			// Set up a path for the object to follow
			pathing->Points.push_back({ 6.0f, 10.0f, 0.0f });
			pathing->Points.push_back({ 6.0f, -4.0f, 0.0f });
			pathing->Speed = 4.0f;
		}

		// Fallen Chicken
		GameObject chicken2 = scene->CreateEntity("chicken_object_2");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Chicken2.obj");
			chicken2.emplace<RendererComponent>().SetMesh(vao).SetMaterial(material1);
			chicken2.get<Transform>().SetLocalPosition(8.0f, 10.0f, 0.0f);
			chicken2.get<Transform>().SetLocalRotation(glm::vec3(45.f, -90.f, 180.f));
			chicken2.get<Transform>().SetLocalScale(glm::vec3(0.4f));
		}

		// Fallen Chicken
		GameObject chicken3 = scene->CreateEntity("chicken_object_3");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Chicken3.obj");
			chicken3.emplace<RendererComponent>().SetMesh(vao).SetMaterial(material1);
			chicken3.get<Transform>().SetLocalPosition(2.0f, 4.0f, 0.0f);
			chicken3.get<Transform>().SetLocalRotation(glm::vec3(50.f, 90.f, 180.f));
			chicken3.get<Transform>().SetLocalScale(glm::vec3(0.4f));
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(chicken3);
		}

		// Spinning chicken
		GameObject chicken4 = scene->CreateEntity("chicken_object_4");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Chicken4.obj");
			chicken4.emplace<RendererComponent>().SetMesh(vao).SetMaterial(material1);
			chicken4.get<Transform>().SetLocalPosition(0.0f, 0.0f, 0.0f);
			chicken4.get<Transform>().SetLocalRotation(glm::vec3(90.f, 0.f, 180.f));
			chicken4.get<Transform>().SetLocalScale(glm::vec3(0.4f));
		}

		// Fallen spinning chicken
		GameObject chicken5 = scene->CreateEntity("chicken_object_5");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Chicken5.obj");
			chicken5.emplace<RendererComponent>().SetMesh(vao).SetMaterial(material1);
			chicken5.get<Transform>().SetLocalPosition(2.0f, 9.0f, 0.0f);
			chicken5.get<Transform>().SetLocalRotation(glm::vec3(0.f, 90.f, 180.f));
			chicken5.get<Transform>().SetLocalScale(glm::vec3(0.4f));
		}

		// Moving chicken right side
		GameObject chicken6 = scene->CreateEntity("chicken_object_6");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Chicken6.obj");
			chicken6.emplace<RendererComponent>().SetMesh(vao).SetMaterial(material1);
			chicken6.get<Transform>().SetLocalPosition(-4.0f, 10.0f, 0.0f);
			chicken6.get<Transform>().SetLocalRotation(glm::vec3(90.f, 0.f, 180.f));
			chicken6.get<Transform>().SetLocalScale(glm::vec3(0.4f));

			// Bind returns a smart pointer to the behaviour that was added
			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(chicken6);
			// Set up a path for the object to follow
			pathing->Points.push_back({ -4.0f, -4.0f, 0.0f });
			pathing->Points.push_back({ -4.0f, 10.0f, 0.0f });
			pathing->Speed = 4.0f;
		}

		// Fallen Chicken
		GameObject chicken7 = scene->CreateEntity("chicken_object_7");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Chicken7.obj");
			chicken7.emplace<RendererComponent>().SetMesh(vao).SetMaterial(material1);
			chicken7.get<Transform>().SetLocalPosition(-7.0f, 8.0f, 0.0f);
			chicken7.get<Transform>().SetLocalScale(glm::vec3(0.4f));
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(chicken7);
		}

		// Spinning chicken upside down
		GameObject chicken8 = scene->CreateEntity("chicken_object_8");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Chicken3.obj");
			chicken8.emplace<RendererComponent>().SetMesh(vao).SetMaterial(material1);
			chicken8.get<Transform>().SetLocalPosition(-7.0f, -3.0f, 3.0f);
			chicken8.get<Transform>().SetLocalRotation(glm::vec3(-90.f, 0.f, 180.f));
			chicken8.get<Transform>().SetLocalScale(glm::vec3(0.4f));
		}

		// Fallen Chicken
		GameObject chicken9 = scene->CreateEntity("chicken_object_9");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Chicken7.obj");
			chicken9.emplace<RendererComponent>().SetMesh(vao).SetMaterial(material1);
			chicken9.get<Transform>().SetLocalPosition(11.0f, -2.0f, 0.0f);
			chicken9.get<Transform>().SetLocalRotation(glm::vec3(90.f, 90.f, 180.f));
			chicken9.get<Transform>().SetLocalScale(glm::vec3(0.4f));
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(chicken9);
		}

		GameObject button = scene->CreateEntity("button_object");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Button.obj");
			button.emplace<RendererComponent>().SetMesh(vao).SetMaterial(material3);
			button.get<Transform>().SetLocalPosition(0.0f, -6.0f, 0.0f);
			button.get<Transform>().SetLocalRotation(glm::vec3(90.f, 0.f, 0.f));
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(button);
		}

		GameObject coil = scene->CreateEntity("coil_object");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Coil.obj");
			coil.emplace<RendererComponent>().SetMesh(vao).SetMaterial(material2);
			coil.get<Transform>().SetLocalPosition(8.0f, -8.0f, 0.0f);
			coil.get<Transform>().SetLocalRotation(glm::vec3(90.f, 0.f, 0.f));
			coil.get<Transform>().SetLocalScale(glm::vec3(0.5f));
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(coil);
		}

		GameObject coil2 = scene->CreateEntity("coil_object_2");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Coil.obj");
			coil2.emplace<RendererComponent>().SetMesh(vao).SetMaterial(material2);
			coil2.get<Transform>().SetLocalPosition(-8.0f, -8.0f, 0.0f);
			coil2.get<Transform>().SetLocalRotation(glm::vec3(90.f, 0.f, 0.f));
			coil2.get<Transform>().SetLocalScale(glm::vec3(0.5f));
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(coil2);
		}
#pragma endregion 

		// Create an object to be our camera
		GameObject cameraObject = scene->CreateEntity("Camera");
		{
			cameraObject.get<Transform>().SetLocalPosition(0, 5, 3).LookAt(glm::vec3(0, 0, 0));

			// We'll make our camera a component of the camera object
			Camera& camera = cameraObject.emplace<Camera>();// Camera::Create();
			camera.SetPosition(glm::vec3(0, 5, 3));
			camera.SetUp(glm::vec3(0, 0, 1));
			camera.LookAt(glm::vec3(0));
			camera.SetFovDegrees(90.0f); // Set an initial FOV
			camera.SetOrthoHeight(3.0f);
			BehaviourBinding::Bind<CameraControlBehaviour>(cameraObject);
		}
		#pragma endregion
		
		//////////////////////////////////////////////////////////////////////////////////////////

		/////////////////////////////////// SKYBOX ///////////////////////////////////////////////
		#pragma region Skybox
		{
			// Load our shaders
			Shader::sptr skybox = std::make_shared<Shader>();
			skybox->LoadShaderPartFromFile("shaders/skybox-shader.vert.glsl", GL_VERTEX_SHADER);
			skybox->LoadShaderPartFromFile("shaders/skybox-shader.frag.glsl", GL_FRAGMENT_SHADER);
			skybox->Link();

			ShaderMaterial::sptr skyboxMat = ShaderMaterial::Create();
			skyboxMat->Shader = skybox;
			skyboxMat->Set("s_Environment", environmentMap);
			skyboxMat->Set("u_EnvironmentRotation", glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0))));
			skyboxMat->RenderLayer = 100;

			MeshBuilder<VertexPosNormTexCol> mesh;
			MeshFactory::AddIcoSphere(mesh, glm::vec3(0.0f), 1.0f);
			MeshFactory::InvertFaces(mesh);
			VertexArrayObject::sptr meshVao = mesh.Bake();

			GameObject skyboxObj = scene->CreateEntity("skybox");
			skyboxObj.get<Transform>().SetLocalPosition(0.0f, 0.0f, 0.0f);
			skyboxObj.get_or_emplace<RendererComponent>().SetMesh(meshVao).SetMaterial(skyboxMat);
		}
		#pragma endregion
		////////////////////////////////////////////////////////////////////////////////////////

		// We'll use a vector to store all our key press events for now (this should probably be a behaviour eventually)
		std::vector<KeyPressWatcher> keyToggles;
		{
			// This is an example of a key press handling helper. Look at InputHelpers.h an .cpp to see
			// how this is implemented. Note that the ampersand here is capturing the variables within
			// the scope. If you wanted to do some method on the class, your best bet would be to give it a method and
			// use std::bind
			keyToggles.emplace_back(GLFW_KEY_T, [&]() { cameraObject.get<Camera>().ToggleOrtho(); });
		}

		// Initialize our timing instance and grab a reference for our use
		Timing& time = Timing::Instance();
		time.LastFrame = glfwGetTime();

		int spinFactor = 0;
		int spinFactor2 = 0;
		int spinFactor3 = 0;

		///// Game loop /////
		while (!glfwWindowShouldClose(BackendHandler::window)) {
			glfwPollEvents();

			// Update the timing
			time.CurrentFrame = glfwGetTime();
			time.DeltaTime = static_cast<float>(time.CurrentFrame - time.LastFrame);

			time.DeltaTime = time.DeltaTime > 1.0f ? 1.0f : time.DeltaTime;

			// Update our FPS tracker data
			fpsBuffer[frameIx] = 1.0f / time.DeltaTime;
			frameIx++;
			if (frameIx >= 128)
				frameIx = 0;

			// We'll make sure our UI isn't focused before we start handling input for our game
			if (!ImGui::IsAnyWindowFocused()) {
				// We need to poll our key watchers so they can do their logic with the GLFW state
				// Note that since we want to make sure we don't copy our key handlers, we need a const
				// reference!
				for (const KeyPressWatcher& watcher : keyToggles) {
					watcher.Poll(BackendHandler::window);
				}
			}

			// Iterate over all the behaviour binding components
			scene->Registry().view<BehaviourBinding>().each([&](entt::entity entity, BehaviourBinding& binding) {
				// Iterate over all the behaviour scripts attached to the entity, and update them in sequence (if enabled)
				for (const auto& behaviour : binding.Behaviours) {
					if (behaviour->Enabled) {
						behaviour->Update(entt::handle(scene->Registry(), entity));
					}
				}
			});

			// Clear the screen
			glClearColor(0.08f, 0.17f, 0.31f, 1.0f);
			glEnable(GL_DEPTH_TEST);
			glClearDepth(1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Update all world matrices for this frame
			scene->Registry().view<Transform>().each([](entt::entity entity, Transform& t) {
				t.UpdateWorldMatrix();
			});

			// Grab out camera info from the camera object
			Transform& camTransform = cameraObject.get<Transform>();
			glm::mat4 view = glm::inverse(camTransform.LocalTransform());
			glm::mat4 projection = cameraObject.get<Camera>().GetProjection();
			glm::mat4 viewProjection = projection * view;

			#pragma region Chicken Updates
			// Rotate chicken when they reach certain y-location
			if (chicken1.get<Transform>().GetLocalPosition().y >= 9.9f)
			{
				chicken1.get<Transform>().SetLocalRotation(glm::vec3(90.f, 0.f, 0.f));
			}
			if (chicken1.get<Transform>().GetLocalPosition().y <= -3.9f)
			{
				chicken1.get<Transform>().SetLocalRotation(glm::vec3(90.f, 0.f, 180.f));
			}

			if (chicken6.get<Transform>().GetLocalPosition().y >= 9.9f)
			{
				chicken6.get<Transform>().SetLocalRotation(glm::vec3(90.f, 0.f, 0.f));
			}
			if (chicken6.get<Transform>().GetLocalPosition().y <= -3.9f)
			{
				chicken6.get<Transform>().SetLocalRotation(glm::vec3(90.f, 0.f, 180.f));
			}

			// Spin chicken4
			chicken4.get<Transform>().SetLocalRotation(glm::vec3(90.0f, 0.0f, spinFactor % 360));
			spinFactor++;

			// Spin chicken5
			chicken5.get<Transform>().SetLocalRotation(glm::vec3(0.0f, 0.0f, spinFactor2 % 360));
			spinFactor2++;

			// Spin chicken 8
			chicken8.get<Transform>().SetLocalRotation(glm::vec3(-90.0f, 0.0f, spinFactor3 % 360));
			spinFactor3++;
#pragma endregion

			// Sort the renderers by shader and material, we will go for a minimizing context switches approach here,
			// but you could for instance sort front to back to optimize for fill rate if you have intensive fragment shaders
			renderGroup.sort<RendererComponent>([](const RendererComponent& l, const RendererComponent& r) {
				// Sort by render layer first, higher numbers get drawn last
				if (l.Material->RenderLayer < r.Material->RenderLayer) return true;
				if (l.Material->RenderLayer > r.Material->RenderLayer) return false;

				// Sort by shader pointer next (so materials using the same shader run sequentially where possible)
				if (l.Material->Shader < r.Material->Shader) return true;
				if (l.Material->Shader > r.Material->Shader) return false;

				// Sort by material pointer last (so we can minimize switching between materials)
				if (l.Material < r.Material) return true;
				if (l.Material > r.Material) return false;

				return false;
			});

			// Start by assuming no shader or material is applied
			Shader::sptr current = nullptr;
			ShaderMaterial::sptr currentMat = nullptr;

			// Iterate over the render group components and draw them
			renderGroup.each([&](entt::entity e, RendererComponent& renderer, Transform& transform) {
				// If the shader has changed, set up it's uniforms
				if (current != renderer.Material->Shader) {
					current = renderer.Material->Shader;
					current->Bind();
					BackendHandler::SetupShaderForFrame(current, view, projection);
				}
				// If the material has changed, apply it
				if (currentMat != renderer.Material) {
					currentMat = renderer.Material;
					currentMat->Apply();
				}
				// Render the mesh
				BackendHandler::RenderVAO(renderer.Material->Shader, renderer.Mesh, viewProjection, transform);
			});

			// Draw our ImGui content
			BackendHandler::RenderImGui();

			scene->Poll();
			glfwSwapBuffers(BackendHandler::window);
			time.LastFrame = time.CurrentFrame;
		}

		// Nullify scene so that we can release references
		Application::Instance().ActiveScene = nullptr;
		BackendHandler::ShutdownImGui();
	}

	// Clean up the toolkit logger so we don't leak memory
	Logger::Uninitialize();
	return 0; 
} 