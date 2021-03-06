#include "PolyRenderingDeviceGLPCH.hpp"

#include "Pipeline/UnlitRenderingPass.hpp"
#include "Proxy/GLMeshDeviceProxy.hpp"
#include "Proxy/GLTextureDeviceProxy.hpp"
#include "GLRenderingDevice.hpp"

using namespace Poly;

UnlitRenderingPass::UnlitRenderingPass(const GLRenderingDevice* rdi)
	: RenderingPassBase(rdi, "Shaders/unlit.vert.glsl", "Shaders/unlit.frag.glsl")
{
	GetProgram().RegisterUniform("mat4", "uTransform");
	GetProgram().RegisterUniform("mat4", "uMVPTransform");
	GetProgram().RegisterUniform("vec4", "Color");
}

void UnlitRenderingPass::OnRun(Scene* world, const CameraComponent* camera, const AARect& /*rect*/, ePassType passType = ePassType::GLOBAL)
{
	GetProgram().BindProgram();
	const Matrix& ScreenFromWorld = camera->GetClipFromWorld();
	
	for (auto componentsTuple : world->IterateComponents<MeshRenderingComponent>())
	{
		const MeshRenderingComponent* meshCmp = std::get<MeshRenderingComponent*>(componentsTuple);

		const EntityTransform& transform = meshCmp->GetTransform();
		const Matrix& WorldFromModel = transform.GetWorldFromModel();
		Matrix ScreenFromModel = ScreenFromWorld * WorldFromModel;
		GetProgram().SetUniform("uTransform", WorldFromModel);
		GetProgram().SetUniform("uMVPTransform", ScreenFromModel);
		
		int i = 0;
		for (const MeshResource::SubMesh* subMesh : meshCmp->GetMesh()->GetSubMeshes())
		{
			Material material = meshCmp->GetMaterial(i);
			GetProgram().SetUniform("uColor", material.Albedo);

			const GLMeshDeviceProxy* meshProxy = static_cast<const GLMeshDeviceProxy*>(subMesh->GetMeshProxy());
			glBindVertexArray(meshProxy->GetVAO());

			const Poly::TextureResource* DiffuseTexture = subMesh->GetMeshData().GetAlbedoMap();
			GLuint TextureID = DiffuseTexture == nullptr
				? RDI->FallbackWhiteTexture
				: static_cast<const GLTextureDeviceProxy*>(DiffuseTexture->GetTextureProxy())->GetTextureID();

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, TextureID);

			glDrawElements(GL_TRIANGLES, (GLsizei)subMesh->GetMeshData().GetTriangleCount() * 3, GL_UNSIGNED_INT, NULL);
			
			glBindTexture(GL_TEXTURE_2D, 0);
			glBindVertexArray(0);

			++i;
		}
	}
}